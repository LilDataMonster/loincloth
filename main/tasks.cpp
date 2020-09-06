#include <esp_event.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_system.h>

// JSON formatting
#include <cJSON.h>

// project headers
#include <sensor.hpp>
#include <http_client.hpp>
#include <tasks.hpp>
#include <ble.hpp>
#include <ota.hpp>

#include <sleep.hpp>
#include <camera.hpp>
#include <cstring>
#include <vector>

#include <led.hpp>
#include <ble.hpp>
#include <system.hpp>

#include <driver/gpio.h>
#include <uri_handles.hpp>

#include <globals.hpp>

#define SLEEP_DURATION CONFIG_SLEEP_DURATION
#define BLE_ADVERTISE_DURATION CONFIG_BLE_ADVERTISE_DURATION

cJSON* json_data = NULL;

// sleep task will go to sleep when messageFinished is true
static bool messageFinished = false;

uint16_t led_fade_time = 3000;
uint16_t led_duty = 4000;
// int32_t led_on = 0;

#define GPIO_OUTPUT_IO_0    LED_GPIO
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO_OUTPUT_IO_0)
// #define GPIO_OUTPUT_IO_1    19
// #define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
void led_on_off_task(void *pvParameters) {

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT; //set as output mode
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL; //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; //disable pull-down mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE; //disable pull-up mode
    gpio_config(&io_conf); //configure GPIO with the given settings

    // int32_t level = 0;
    while(true) {
        gpio_set_level(LED_GPIO, led_on%2);
        ESP_LOGI("LDM:LED", "LED: %d, Period Enabled: %s, Period: %u ms",
                  led_on%2, (is_period_enabled?"True":"False"), led_period_ms);
        // led_on += 1;
        // vTaskDelay(pdMS_TO_TICKS(10000));
        led_on += is_period_enabled ? 1 : 0;
        vTaskDelay(pdMS_TO_TICKS(led_period_ms));
    }
}

void led_fade_task(void *pvParameters) {
    ledc_timer_config_t ledc_timer;
    ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
    ledc_timer.freq_hz = 5000;                      // frequency of PWM signal
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;    // timer mode
    ledc_timer.timer_num = LEDC_TIMER_1;            // timer index
    ledc_timer.clk_cfg = LEDC_AUTO_CLK;             // Auto select the source clock

    ledc_channel_config_t backlight;
    backlight.channel    = LEDC_CHANNEL_0;
    backlight.duty       = 0;
    backlight.gpio_num   = 14;
    backlight.speed_mode = LEDC_LOW_SPEED_MODE;
    backlight.hpoint     = 0;
    backlight.timer_sel  = LEDC_TIMER_1;

    LDM::LED led;
    led.configLedTimer(ledc_timer);
    led.addLedChannelConfig(backlight);
    led.init();

    while(1) {
        led.fadeLedWithTime(0);
        vTaskDelay(led_fade_time / portTICK_PERIOD_MS);

        led.fadeLedWithTime(0, 0);
        vTaskDelay(led_fade_time / portTICK_PERIOD_MS);

        led.setDuty(0, led_duty);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        led.setDuty(0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


#define SENSOR_TASK_LOG "SENSOR_TASK"
void sensor_task(void *pvParameters) {
// void sensor_task(std::vector<LDM::Sensor>& sensors) {

    std::vector<LDM::Sensor*> const *sensors = reinterpret_cast<std::vector<LDM::Sensor*> const*>(pvParameters);

    for(auto const& sensor : *sensors) {
        ESP_LOGI(SENSOR_TASK_LOG, "Initializing Sensor: %s", sensor->getSensorName());
        sensor->init();
    }

    while(true){
        if(json_data != NULL) {
            cJSON_Delete(json_data);
            json_data = NULL;
        }
        json_data = cJSON_CreateObject();
        for(auto const& sensor : *sensors) {
            ESP_LOGI(SENSOR_TASK_LOG, "Reading Sensor: %s", sensor->getSensorName());
            sensor->readSensor();
            cJSON *sensor_json = sensor->buildJson();
            cJSON_AddItemToObject(json_data, sensor->getSensorName(), sensor_json);
            sensor->releaseData();

            // char* sensor_out = cJSON_Print(sensor_json);
            // printf("%s\n", sensor_out);
            // free(sensor_out);
        }

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        // vTaskDelay(2000 / portTICK_PERIOD_MS);
        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    // if(pvParameters == NULL) {
    //     ESP_LOGE(SENSOR_TASK_LOG, "Invalid Sensor Recieved");
    //     return;
    // }
    //
    // std::vector<LDM::Sensor*> const *sensors = reinterpret_cast<std::vector<LDM::Sensor*> const*>(pvParameters);
    //
    // for(auto const& sensor : *sensors) {
    //     if(sensor->init() != ESP_OK) {
    //         ESP_LOGE(SENSOR_TASK_LOG, "Failed to initialize sensor");
    //         return;
    //     }
    // }
    //
    // while(true){
    //     for(auto const& sensor : *sensors) {
    //         sensor->readSensor();
    //     }
    //     // If you read the sensor data too often, it will heat up
    //     // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
    //     // vTaskDelay(2000 / portTICK_PERIOD_MS);
    //     vTaskDelay(pdMS_TO_TICKS(10000));
    // }
}

#define HTTP_POST_ENDPOINT CONFIG_ESP_POST_ENDPOINT
#define HTTP_TASK_LOG "HTTP_TASK"
//#define GPIO_OUTPUT_PIN 13
//#define GPIO_OUTPUT_PIN_SEL  (1ULL << GPIO_OUTPUT_PIN)
#define FIRMWARE_UPGRADE_ENDPOINT CONFIG_FIRMWARE_UPGRADE_ENDPOINT
void http_task(void *pvParameters) {

    // setup http client
    char* endpoint_url = NULL;
    size_t endpoint_url_size = 0;

    // check key and get url size if it exists
    if(g_nvs != NULL) {
        g_nvs->openNamespace("url");
        esp_err_t err = g_nvs->getKeyStr("post", NULL, &endpoint_url_size);
        if(err != ESP_OK) {
            endpoint_url = const_cast<char*>(HTTP_POST_ENDPOINT);
            if(err == ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGI(HTTP_TASK_LOG, "No Post URL found in NVS, setting to default: %s", endpoint_url);
            }
        } else {
            endpoint_url = (char*)malloc(endpoint_url_size);
            err = g_nvs->getKeyStr("post", endpoint_url, &endpoint_url_size);
            if(err != ESP_OK) {
                free(endpoint_url);
                endpoint_url = const_cast<char*>(HTTP_POST_ENDPOINT);
                ESP_LOGI(HTTP_TASK_LOG, "Error fetching Post URL from NVS, setting to default: %s", endpoint_url);
            }
        }
        g_nvs->close();
    } else {
        endpoint_url = const_cast<char*>(HTTP_POST_ENDPOINT);
        ESP_LOGI(HTTP_TASK_LOG, "No NVS found when fetching Post URL from NVS, setting to default: %s", endpoint_url);
    }

    LDM::HTTP_Client http(endpoint_url);
    g_http_client = &http;

#ifdef CONFIG_OTA_ENABLED
    // setup ota updater and checkUpdates
    LDM::OTA ota(const_cast<char*>(FIRMWARE_UPGRADE_ENDPOINT));
#endif

    while(true) {
        if(g_ble->wifi.isConnected()) {
            if(json_data != NULL) {
                // char* post_data = cJSON_Print(json_data);
                // ESP_LOGI(HTTP_TASK_LOG, "%s", post_data);

                // // POST
                http.postJSON(json_data);
                // http.postFormattedJSON(post_data);
                // free(post_data);
            } else {
                ESP_LOGI(HTTP_TASK_LOG, "SENSOR_JSON value is NULL");
            }
#ifdef CONFIG_OTA_ENABLED
            // check OTA updates
            ota.checkUpdates(true);
#endif
        } else {
            ESP_LOGI(HTTP_TASK_LOG, "Wifi is not connected");
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
    // // // cleanup JSON message
    // // cJSON_Delete(message);
    // // message = NULL;

    // vEventGroupDelete(s_wifi_event_group);
    // wifi.deinit_sta();
    http.deinit();

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
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

// void blufi_task(void *pvParameters) {
//     LDM::BLE ble_dev(const_cast<char*>("BLUFI_TEST"));
//     ble_dev.init();
//     ble_dev.setupDefaultBlufiCallback();
//     ble_dev.initBlufi();
//     g_ble = &ble_dev;
//     while(true) {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

#ifndef CONFIG_IDF_TARGET_ESP32S2
#define BLE_TASK_LOG "BLE_TASK"
void ble_task(void *pvParameters) {
//     ESP_LOGI(BLE_TASK_LOG, "Starting BLE");
//
//     LDM::BLE ble("Nightgown");
//     ble.init();
// /*
//     ble.setupCallback();
//
// #if CONFIG_DHT11_SENSOR_ENABLED
//     // get sensor data
//     //LDM::DHT* dht_sensor = (LDM::DHT*)pvParameters;
//     LDM::DHT* p_dht_sensor = &dht_sensor;
//
//     uint8_t humidity = p_dht_sensor->getHumidity();
//     uint8_t temperature = p_dht_sensor->getTemperature();
//     ESP_LOGI(BLE_TASK_LOG, "Updating humidity: %d, temperature: %d", humidity, temperature);
//     ble.updateValue(humidity, temperature);
// #endif
// */
//
//     // advertise BLE data for a while
//     vTaskDelay(pdMS_TO_TICKS(BLE_ADVERTISE_DURATION * 1E3));
//     ble.deinit();
    messageFinished = true;
    vTaskDelete(NULL);
}
#endif
