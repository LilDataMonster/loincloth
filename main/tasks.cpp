#include <esp_event.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_system.h>

// JSON formatting
#include <cJSON.h>

// project headers
#include <sensor.hpp>
#include <http.hpp>
#include <tasks.hpp>
#include <wifi.hpp>
#include <ble.hpp>
#include <ota.hpp>
#include <server.hpp>

#include <sleep.hpp>
#include <camera.hpp>
#include <cstring>
#include <vector>

#include <uri_handles.hpp>

#define SLEEP_DURATION CONFIG_SLEEP_DURATION
#define BLE_ADVERTISE_DURATION CONFIG_BLE_ADVERTISE_DURATION

// sleep task will go to sleep when messageFinished is true
static bool messageFinished = false;

#define SENSOR_TASK_LOG "SENSOR_TASK"
void sensor_task(void *pvParameters) {
// void sensor_task(std::vector<LDM::Sensor>& sensors) {

    if(pvParameters == NULL) {
        ESP_LOGE(SENSOR_TASK_LOG, "Invalid Sensor Recieved");
        return;
    }

    std::vector<LDM::Sensor*> const *sensors = reinterpret_cast<std::vector<LDM::Sensor*> const*>(pvParameters);

    for(auto const& sensor : *sensors) {
        if(sensor->init() != ESP_OK) {
            ESP_LOGE(SENSOR_TASK_LOG, "Failed to initialize sensor");
            return;
        }
    }

    while(true){
        for(auto const& sensor : *sensors) {
            sensor->readSensor();
        }
        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

#define HTTP_POST_ENDPOINT CONFIG_ESP_POST_ENDPOINT
#define HTTP_TASK_LOG "HTTP_TASK"
//#define GPIO_OUTPUT_PIN 13
//#define GPIO_OUTPUT_PIN_SEL  (1ULL << GPIO_OUTPUT_PIN)
#define FIRMWARE_UPGRADE_ENDPOINT CONFIG_FIRMWARE_UPGRADE_ENDPOINT
void http_task(void *pvParameters) {
    // gpio_config_t io_conf;
    // //disable interrupt
    // io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // //set as output mode
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // //bit mask of the pins that you want to set,e.g.GPIO18/19
    // io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // //disable pull-down mode
    // io_conf.pull_down_en = 0;
    // //disable pull-up mode
    // io_conf.pull_up_en = 0;
    // //configure GPIO with the given settings
    // gpio_config(&io_conf);
    //
    // gpio_set_level(GPIO_OUTPUT_PIN, 1);

    if(pvParameters == NULL) {
        ESP_LOGE(HTTP_TASK_LOG, "Invalid Sensor Recieved");
        return;
    }

    // setup wifi and http client
    LDM::WiFi wifi;
    LDM::HTTP http(const_cast<char*>(HTTP_POST_ENDPOINT));

#ifdef CONFIG_OTA_ENABLED
    // setup ota updater and checkUpdates
    LDM::OTA ota(const_cast<char*>(FIRMWARE_UPGRADE_ENDPOINT));
#endif

    // // LDM::Sensor *sensor = (LDM::Sensor*)pvParameters;
    // std::vector<LDM::Sensor*> const *sensors = reinterpret_cast<std::vector<LDM::Sensor*> const*>(pvParameters);

    char* post_data = (char*)pvParameters;
    ESP_LOGI(HTTP_TASK_LOG, "%s", post_data);

    // sensors->at(0)->init();
    // sensors->at(0)->readSensor();
    // cJSON *message = sensors->at(0)->buildJson();
    // char* output = cJSON_Print(message);
    // printf("%s", output);
    // for(auto const& sensor : *sensors) {
    //     sensor->readSensor();
    // }
    wifi.init_sta();

    LDM::Server server("");
    server.startServer();
    server.registerUriHandle(&uri_get);
    server.registerUriHandle(&uri_post);

    // create JSON message
    // cJSON *message = sensor->buildJson();
    // cJSON *message = sensors->at(0)->buildJson();
    //
    // if(message != NULL) {
    //     char* output = cJSON_Print(message);
    //     printf("%s", output);
    // }

    // cJSON *message = cJSON_CreateObject();
    // for(auto const& sensor : *sensors) {
    //     printf("Building Sensor JSON\n");
    //     cJSON *json_sensor = sensor->buildJson();
    // }
    // for(auto const& sensor : *sensors) {
    //     cJSON *json_sensor = sensor->buildJson();
    //     // cJSON_AddItemToObject(message, sensor->getSensorName(), json_sensor);
    //
    //     // printf("%s", sensor->getSensorName());
    //
    //     char* output = cJSON_Print(json_sensor);
    //     if(output != NULL) {
    //         printf("%s", output);
    //     } else {
    //         printf("cJSON_Print output is NULL!\n");
    //     }
    //     cJSON_AddItemToObject(message, "sensor", json_sensor);
    // }

    // char* output = cJSON_Print(message);
    // printf("%s", output);

    // POST
    // http.postJSON(message);
    http.postFormattedJSON(post_data);

#ifdef CONFIG_OTA_ENABLED
    // check OTA updates
    ota.checkUpdates(true);
#endif

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    // // // cleanup JSON message
    // // cJSON_Delete(message);
    // // message = NULL;
    //
    // // vEventGroupDelete(s_wifi_event_group);
    // wifi.deinit_sta();
    // http.deinit();
    //
    // messageFinished = true;
    // vTaskDelete(NULL);
}

#define SLEEP_TASK_LOG "SLEEP_TASK"
void sleep_task(void *pvParameters) {
    std::vector<LDM::Sensor*> const *sensors = reinterpret_cast<std::vector<LDM::Sensor*> const*>(pvParameters);

    while(true) {
        if(messageFinished) {
            // deinitialize sensors
            for(auto const& sensor : *sensors) {
                sensor->deinit();
            }

            // enter deep sleep
            LDM::Sleep::enterDeepSleepSec(SLEEP_DURATION);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

#ifndef CONFIG_IDF_TARGET_ESP32S2
#define BLE_TASK_LOG "BLE_TASK"
void ble_task(void *pvParameters) {
    ESP_LOGI(BLE_TASK_LOG, "Starting BLE");

    LDM::BLE ble("Nightgown");
    ble.init();
/*
    ble.setupCallback();

#if CONFIG_DHT11_SENSOR_ENABLED
    // get sensor data
    //LDM::DHT* dht_sensor = (LDM::DHT*)pvParameters;
    LDM::DHT* p_dht_sensor = &dht_sensor;

    uint8_t humidity = p_dht_sensor->getHumidity();
    uint8_t temperature = p_dht_sensor->getTemperature();
    ESP_LOGI(BLE_TASK_LOG, "Updating humidity: %d, temperature: %d", humidity, temperature);
    ble.updateValue(humidity, temperature);
#endif
*/

    // advertise BLE data for a while
    vTaskDelay(pdMS_TO_TICKS(BLE_ADVERTISE_DURATION * 1E3));
    ble.deinit();
    messageFinished = true;
    vTaskDelete(NULL);
}
#endif
