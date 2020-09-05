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

#define APP_TAG "LOINCLOTH"

// enum BoardMode { setup, sleep };
// static BoardMode mode = setup;

#include <nvs.hpp>
#include <sleep.hpp>
#include <system.hpp>
#include <server.hpp>
#include <sensors.hpp>
#include <ble.hpp>

#include <uri_handles.hpp>

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
LDM::BLE *g_ble;
LDM::Server *g_http_server;
LDM::System *g_system;
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

#define APP_MAIN "LDM:Main"
void app_main(void) {

    LDM::Sleep::getWakeupCause();

    // open the "broadcast" key-value pair from the "state" namespace in NVS
    uint8_t broadcast = 0; // value will default to 0, if not set yet in NVS

    // initialize nvs
    LDM::NVS nvs;
    nvs.openNamespace("state");
    nvs.getKeyU8("broadcast", &broadcast);
    broadcast++;
    nvs.setKeyU8("broadcast", broadcast);
    nvs.commit();
    nvs.close();

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

    LDM::BLE ble_dev(const_cast<char*>("BLUFI_TEST"));
    ble_dev.init();
    ble_dev.setupDefaultBlufiCallback();
    ble_dev.initBlufi();
    g_ble = &ble_dev;

    LDM::Server server(const_cast<char*>(""));
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

    while(true) {
        if(g_ble->wifi.isConnected() && !g_http_server->isStarted()) {
            g_http_server->startServer();
            g_http_server->registerUriHandle(&uri_get);
            g_http_server->registerUriHandle(&uri_post);
            g_http_server->registerUriHandle(&uri_data);
            g_http_server->registerUriHandle(&uri_get_camera);
            g_http_server->registerUriHandle(&uri_post_camera);
            g_http_server->registerUriHandle(&uri_options_camera);
            g_http_server->registerUriHandle(&uri_get_stream);
            g_http_server->registerUriHandle(&uri_post_led);
        }

        if(json_data != NULL) {
            cJSON_Delete(json_data);
            json_data = NULL;
        }
        json_data = cJSON_CreateObject();
        for(auto const& sensor : sensors) {
            ESP_LOGI(APP_MAIN, "Reading Sensor: %s", sensor->getSensorName());
            sensor->readSensor();
            cJSON *sensor_json = sensor->buildJson();
            cJSON_AddItemToObject(json_data, sensor->getSensorName(), sensor_json);
            sensor->releaseData();

            // char* sensor_out = cJSON_Print(sensor_json);
            // printf("%s\n", sensor_out);
            // free(sensor_out);
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}
