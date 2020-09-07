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

// enum BoardMode { setup, sleep };
// static BoardMode mode = setup;

#include <nvs.hpp>
#include <sleep.hpp>
#include <system.hpp>
#include <http_server.hpp>
#include <ble.hpp>

#include <uri_handles.hpp>
#include <globals.hpp>

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
LDM::Camera camera = LDM::Camera(FRAMESIZE_QCIF, PIXFORMAT_JPEG, 10, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_HVGA, PIXFORMAT_JPEG, 10, 1);

// LDM::Camera camera = LDM::Camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 50, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_HVGA, PIXFORMAT_JPEG, 20, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 30, 1);
// LDM::Camera camera = LDM::Camera(FRAMESIZE_CIF, PIXFORMAT_JPEG, 50, 1);
#endif

// define extern variables
cJSON * json_system = NULL;
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

#define DEFAULT_SSID CONFIG_WIFI_SSID
#define DEFAULT_PWD CONFIG_WIFI_PASSWORD
#define APP_MAIN "LDM:Main"
void app_main(void) {

    esp_err_t err = ESP_OK;
    LDM::Sleep::getWakeupCause();

    // open the "broadcast" key-value pair from the "state" namespace in NVS
    uint8_t broadcast = 0; // value will default to 0, if not set yet in NVS

    wifi_config_t wifi_config = {};
    size_t wifi_size = 0;

    // initialize nvs
    LDM::NVS nvs;
    g_nvs = &nvs;
    g_nvs->openNamespace("system");

    // load/update broadcast
    g_nvs->getKeyU8("broadcast", &broadcast);
    broadcast++;
    g_nvs->setKeyU8("broadcast", broadcast);
    g_nvs->commit();

    uint8_t ssid[32];
    uint8_t passwd[64];
    // load wifi settings
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

    // sensors.at(0)->init();
    json_system = cJSON_CreateObject();

    LDM::System system;
    cJSON *system_json = system.buildJson();
    cJSON_AddItemToObject(json_data, APP_TAG, system_json);

    // for(auto const& sensor : sensors) {
    //     printf("Processing sensor %s\n", sensor->getSensorName());
    //     sensor->init();
    //     sensor->readSensor();
    //     cJSON *sensor_json = sensor->buildJson();
    //     char* sensor_out = cJSON_Print(sensor_json);
    //     printf("%s\n", sensor_out);
    //     free(sensor_out);
    //     cJSON_AddItemToObject(json_data, sensor->getSensorName(), sensor_json);
    // }

    // char* output = cJSON_Print(json_data);

    // if(mode == BoardMode::setup) {
    //     ESP_LOGI(APP_TAG, "Board in setup mode");
    // }

    // wifi_config_t w_config = {};
    // std::strcpy((char*)w_config.sta.ssid, "DavidsTheBest");
    // std::strcpy((char*)w_config.sta.password, "Besties2");
    // std::strcpy((char*)w_config.sta.ssid, (char*)ssid);
    // std::strcpy((char*)w_config.sta.password, (char*)passwd);

    // LDM::WiFi wifi;
    // wifi.init(&w_config);

    LDM::BLE ble_dev(const_cast<char*>("BLUFI_TEST"));
    ble_dev.init();
    ble_dev.setupDefaultBlufiCallback();
    // ble_dev.initBlufi(&w_config);
    ble_dev.initBlufi(&wifi_config);
    // ble_dev.initBlufi();
    g_ble = &ble_dev;

    LDM::HTTP_Server server(const_cast<char*>(""));
    g_http_server = &server;

    httpd_config_t * server_config = g_http_server->getConfig();
    server_config->send_wait_timeout = 20;

    // xTaskCreate(sensor_task, "sensor_task", 8192, (void*)&sensors, 5, NULL);
    xTaskCreate(http_task, "http_task", 8192, NULL, 5, NULL);

    // xTaskCreate(http_task, "http_task", 8192, (void*)output, 5, NULL);
    // xTaskCreate(http_task, "http_task", 8192, NULL, 5, NULL);
    // xTaskCreate(led_fade_task, "led_task", 3*configMINIMAL_STACK_SIZE, NULL, 5, NULL);
    xTaskCreate(led_on_off_task, "led_task", 3*configMINIMAL_STACK_SIZE, NULL, 5, NULL);

    // // setup watcher for sleep
    // xTaskCreate(sleep_task, "sleep_task", configMINIMAL_STACK_SIZE, (void*)&sensors, 5, NULL);

    for(auto const& sensor : sensors) {
        ESP_LOGI(APP_MAIN, "Initializing Sensor: %s", sensor->getSensorName());
        sensor->init();
    }

    // ble_dev.wifi.disconnect();
    // ble_dev.wifi.connect();
    while(true) {
        // TODO: Handle disconnect/stopping server
        if(g_ble->wifi.isConnected() && !g_http_server->isStarted()) {
        // if(!g_http_server->isStarted()) {
            g_http_server->startServer();
            g_http_server->registerUriHandle(&uri_get);
            g_http_server->registerUriHandle(&uri_post);
            g_http_server->registerUriHandle(&uri_data);
            g_http_server->registerUriHandle(&uri_get_camera);
            g_http_server->registerUriHandle(&uri_post_config);
            g_http_server->registerUriHandle(&uri_options_config);
            g_http_server->registerUriHandle(&uri_get_stream);
        }

        if(json_data != NULL) {
            cJSON_Delete(json_data);
            json_data = NULL;
        }
        json_data = cJSON_CreateObject();
        for(auto const& sensor : sensors) {
            ESP_LOGI(APP_MAIN, "Reading Sensor: %s", sensor->getSensorName());
            if(std::strcmp(sensor->getSensorName(), "Camera") == 0 && is_camera_led_flash_enabled) {
                gpio_set_level(LED_GPIO, 1);
                vTaskDelay(150 / portTICK_PERIOD_MS);
            }
            sensor->readSensor();
            if(std::strcmp(sensor->getSensorName(), "Camera") == 0 &&
               !led_on &&
               is_camera_led_flash_enabled) {
                gpio_set_level(LED_GPIO, 0);
            }
            cJSON *sensor_json = sensor->buildJson();
            cJSON_AddItemToObject(json_data, sensor->getSensorName(), sensor_json);
            sensor->releaseData();

            // char* sensor_out = cJSON_Print(sensor_json);
            // printf("%s\n", sensor_out);
            // free(sensor_out);
        }

        if(g_nvs != NULL) {
            // get current wifi config
            wifi_config_t wifi_config_tmp;
            err |= g_ble->wifi.getConfig(ESP_IF_WIFI_STA, &wifi_config_tmp);

            // update NVS ssid and password if different from initial ssid/password
            ESP_LOGI(APP_MAIN, "Comparing SSID: %s to tmp_SSID: %s", wifi_config.sta.ssid, wifi_config_tmp.sta.ssid);
            ESP_LOGI(APP_MAIN, "Comparing password: %s to tmp_password: %s", wifi_config.sta.password, wifi_config_tmp.sta.password);
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
