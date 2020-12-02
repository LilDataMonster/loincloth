#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_pm.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <time.h>
#include <sys/time.h>
#include <tasks.hpp>
#include <vector>
#include <cstring>

#define APP_TAG "LOINCLOTH"

// TODO: setup different board modes
// enum BoardMode { setup, sleep };
// static BoardMode mode = setup;

#include <nvs.hpp>
#include <sleep.hpp>
#include <system.hpp>
#include <http_server.hpp>
#include <ble.hpp>

#include <uri_handles.hpp>
#include <globals.hpp>
#include <ble_services.hpp>

extern "C" {
void app_main(void);
}

#if CONFIG_DHT_SENSOR_ENABLED
#include <dht.hpp>
LDM::DHT dht;
#endif

#if CONFIG_BME680_SENSOR_ENABLED
#include <bme680.hpp>
LDM::BME680 bme680;
#endif

#if CONFIG_CAMERA_SENSOR_ENABLED
#include <camera.hpp>

// setup camera image resolution/configufation
LDM::Camera camera = LDM::Camera(FRAMESIZE_HVGA, PIXFORMAT_JPEG, 10, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_QCIF, PIXFORMAT_JPEG, 10, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 10, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 50, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_HVGA, PIXFORMAT_JPEG, 20, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 30, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_CIF, PIXFORMAT_JPEG, 50, 1);
#endif

// define extern variables
LDM::NVS *g_nvs = NULL;
LDM::BLE *g_ble = NULL;
LDM::HTTP_Server *g_http_server = NULL;
LDM::HTTP_Client *g_http_client = NULL;
LDM::System *g_system = NULL;

std::vector<LDM::Sensor*> sensors {
#if CONFIG_DHT_SENSOR_ENABLED
    &dht,
#endif
#if CONFIG_BME680_SENSOR_ENABLED
    &bme680,
#endif
#if CONFIG_CAMERA_SENSOR_ENABLED
    &camera,
#endif
};

uint8_t mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t ipv4[4] = {0x00, 0x00, 0x00, 0x00};

// define default wifi parameters from kconfig
#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD
#define APP_MAIN "LDM:Main"

void app_main(void) {

    esp_err_t err = ESP_OK;

    // report reason for last power-off to console
    LDM::Sleep::getWakeupCause();

    // open the "broadcast" key-value pair from the "state" namespace in NVS
    uint8_t broadcast = 0; // value will default to 0, if not set yet in NVS

    // initialize wifi configurations
    wifi_config_t wifi_config = {};
    size_t wifi_size = 0;

    // initialize nvs
    LDM::NVS nvs;
    g_nvs = &nvs;
    g_nvs->openNamespace("system");

    // load/update 'broadcast' variable in NVS
    g_nvs->getKeyU8("broadcast", &broadcast);
    broadcast++;
    g_nvs->setKeyU8("broadcast", broadcast);
    g_nvs->commit();

    uint8_t ssid[32];
    uint8_t passwd[64];

    // load wifi settings from NVS memory (or set default if wifi settings don't exist)
    err = g_nvs->getKeyStr("wifi_ssid", NULL, &wifi_size);      // fetch wifi ssid size (max 32)
    if(err == ESP_OK) {
        // g_nvs->getKeyStr("wifi_ssid", (char*)ssid, &wifi_size);
        g_nvs->getKeyStr("wifi_ssid", (char*)wifi_config.sta.ssid, &wifi_size);
        // g_nvs->getKeyStr("wifi_password", NULL, &wifi_size);  // fetch wifi ssid size (max 64)
        // g_nvs->getKeyStr("wifi_password", (char*)passwd, &wifi_size);
        g_nvs->getKeyStr("wifi_password", (char*)wifi_config.sta.password, &wifi_size);
    } else {
        std::strcpy((char*)ssid, DEFAULT_SSID);
        std::strcpy((char*)passwd, DEFAULT_PWD);
        // std::strcpy((char*)wifi_config.sta.ssid, DEFAULT_SSID);
        // std::strcpy((char*)wifi_config.sta.password, DEFAULT_PWD);
    }
    g_nvs->close();

    // setup MAC for broadcasting
    err = esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // TODO: Add different modes of which the board can be in
    // if(mode == BoardMode::setup) {
    //     ESP_LOGI(APP_TAG, "Board in setup mode");
    // }

    // initialize bluetooth device
    LDM::BLE ble_dev(const_cast<char*>(CONFIG_BLUETOOTH_DEVICE_NAME));
    ble_dev.init();                                           // initialize bluetooth
    ble_dev.setupDefaultBlufiCallback();                      // setup blufi configuration
    ble_dev.initBlufi(&wifi_config);                          // initialize blufi with given wifi configuration
    ble_dev.registerGattServerCallback(gatts_event_handler);  // setup ble gatt server callback handle
    ble_dev.registerGattServerAppId(ESP_APP_ID);              // setup ble gatt application profile from database
    g_ble = &ble_dev;

    // initialize sensor
    for(auto const& sensor : sensors) {
        ESP_LOGI(APP_MAIN, "Initializing Sensor: %s", sensor->getSensorName());
        sensor->init();
    }

    // setup http server for modifying devices using REST calls (LEDs, camera, etc.)
    LDM::HTTP_Server server(const_cast<char*>(""));
    g_http_server = &server;
    httpd_config_t * server_config = g_http_server->getConfig();
    server_config->send_wait_timeout = 20;

    // setup RTOS tasks
    // xTaskCreate(sleep_task, "sleep_task", configMINIMAL_STACK_SIZE, (void*)&sensors, 5, NULL); // task: watcher for initiating sleeps
    // xTaskCreate(sensor_task, "sensor_task", 8192, (void*)&sensors, 5, NULL);                   // task: sensor data (moved into main.cpp)
    xTaskCreate(http_task, "http_task", 8192, NULL, 5, NULL);                                     // task: publishing data with REST POST
    xTaskCreate(xbee_task, "xbee_task", 8192, NULL, 5, NULL); 

    // xTaskCreate(led_fade_task, "led_task", 3*configMINIMAL_STACK_SIZE, NULL, 5, NULL);         // task: fade LED lights
    xTaskCreate(led_on_off_task, "led_task", 3*configMINIMAL_STACK_SIZE, NULL, 5, NULL);          // task: turn on/off LED lights (no fade)

    // sensor task moved here
    while(true) {

        // update bluetooth characteristic with latest ipv4 address
        if(ble_dev.wifi.getIpv4(ipv4) != ESP_OK) {
            ESP_LOGE(APP_MAIN, "Unable to fetch IPV4");
            ipv4[0] = 0;
            ipv4[1] = 0;
            ipv4[2] = 0;
            ipv4[3] = 0;
        } else {
            ESP_LOGI(APP_MAIN, "Fetched IPV4: %x:%x:%x:%x", ipv4[0], ipv4[1], ipv4[2], ipv4[3]);
            err = bleUpdateIpv4();
            if(err != ESP_OK) {
                ESP_LOGE(APP_MAIN, "Unable to update IPV4 Bluetooth characteristic");
            }
        }

        // start onboard HTTP server and register URI target locations for REST handles
        // TODO: Handle disconnect/stopping server
        if(g_ble->wifi.isConnected() && !g_http_server->isStarted()) {
            g_http_server->startServer();
            if(g_http_server->isStarted()) {
                ESP_LOGI(APP_MAIN, "Registering HTTP Server URI Handles");
                g_http_server->registerUriHandle(&uri_get);
                g_http_server->registerUriHandle(&uri_post);
                g_http_server->registerUriHandle(&uri_data);
                g_http_server->registerUriHandle(&uri_get_camera);
                g_http_server->registerUriHandle(&uri_post_config);
                g_http_server->registerUriHandle(&uri_options_config);
                g_http_server->registerUriHandle(&uri_get_stream);
            }
        }

        // create a fresh JSON buffer (delete/free existing buffer if one already exists)
        if(json_data != NULL) {
            cJSON_Delete(json_data);
            json_data = NULL;
        }
        json_data = cJSON_CreateObject();

        // construct JSON data for system information
        LDM::System system;
        cJSON *system_json = system.buildJson();
        cJSON_AddItemToObject(json_data, APP_TAG, system_json);

        // loop through each sensor and collect readings
        for(auto const& sensor : sensors) {
            ESP_LOGI(APP_MAIN, "Reading Sensor: %s", sensor->getSensorName());

            // if sensor is a camera and camera flash is enabled, start LED light
            if(std::strcmp(sensor->getSensorName(), "Camera") == 0 && is_camera_led_flash_enabled) {
                gpio_set_level(LED_GPIO, 1);
                vTaskDelay(150 / portTICK_PERIOD_MS);  // wait a little to allow LED to flash (otherwise camera captures image too quickly)
            }

            // read and update sensor data
            sensor->readSensor();

            // if sensor is a camera and camera flash is enabled, stop LED light
            if(std::strcmp(sensor->getSensorName(), "Camera") == 0 &&
               !led_on &&
               is_camera_led_flash_enabled) {
                gpio_set_level(LED_GPIO, 0);
            }

            // TODO: Look into proper memory/heap allocation for camera data to place into a task
            // if sensor is a camera, build the JSON data for its hex64 encoded representation (memory intensive)
            if(std::strcmp(sensor->getSensorName(), "Camera") != 0) {
                cJSON *sensor_json = sensor->buildJson();
                cJSON_AddItemToObject(json_data, sensor->getSensorName(), sensor_json);
                sensor->releaseData();
            }

            // char* sensor_out = cJSON_Print(sensor_json);
            // printf("%s\n", sensor_out);
            // free(sensor_out);
        }

#if CONFIG_DHT_SENSOR_ENABLED
        // update bluetooth output with DHT sensor data
        err = bleUpdateDht();
        if(err != ESP_OK) {
            ESP_LOGE(APP_MAIN, "Unable to update DHT Bluetooth characteristic");
        }
#endif

#if CONFIG_BME680_SENSOR_ENABLED
        // update bluetooth output BME680 sensor data
        err = bleUpdateBme680();
        if(err != ESP_OK) {
            ESP_LOGE(APP_MAIN, "Unable to update BME680 Bluetooth characteristic");
        }
#endif

        if(g_nvs != NULL) {
            // get current wifi config
            wifi_config_t wifi_config_tmp;
            err |= g_ble->wifi.getConfig(ESP_IF_WIFI_STA, &wifi_config_tmp);

            // update NVS memory with ssid and password if different from initial ssid/password
            // this allows for the device to remember WIFI/PASSWORD configurations if power is disconnected

            // ESP_LOGI(APP_MAIN, "Comparing SSID: %s to tmp_SSID: %s", wifi_config.sta.ssid, wifi_config_tmp.sta.ssid);
            // ESP_LOGI(APP_MAIN, "Comparing password: %s to tmp_password: %s", wifi_config.sta.password, wifi_config_tmp.sta.password);
            if(std::strcmp((char*)wifi_config.sta.ssid, (char*)wifi_config_tmp.sta.ssid) != 0 &&
               std::strcmp((char*)wifi_config.sta.password, (char*)wifi_config_tmp.sta.password) != 0){
               err = g_nvs->openNamespace("system");
               if(err == ESP_OK) {
                   err |= g_nvs->setKeyStr("wifi_ssid", reinterpret_cast<char*>(wifi_config_tmp.sta.ssid));
                   err |= g_nvs->setKeyStr("wifi_password", reinterpret_cast<char*>(wifi_config_tmp.sta.password));
                   err |= g_nvs->commit();
                   g_nvs->close();

                   ESP_LOGI(APP_MAIN, "Updated wifi settings in NVS");
                   std::strcpy((char*)wifi_config.sta.ssid, (char*)wifi_config_tmp.sta.ssid);
                   std::strcpy((char*)wifi_config.sta.password, (char*)wifi_config_tmp.sta.password);
               }
               if(err != ESP_OK) {
                   ESP_LOGE(APP_MAIN, "Error saving wifi parameters to NVS");
               }
            }
        }

        // sleep
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
