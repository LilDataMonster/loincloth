#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_pm.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <time.h>
#include <sys/time.h>
#include <tasks.hpp>

#define APP_TAG "LOINCLOTH"

#if CONFIG_DHT_SENSOR_ENABLED
#include <dht.hpp>
static LDM::DHT sensor;
#endif

#if CONFIG_BME680_SENSOR_ENABLED
#include <bme680.hpp>
static LDM::BME680 sensor;
#endif

extern "C" {
void app_main(void);
}

static RTC_DATA_ATTR struct timeval sleep_enter_time;

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // get cause of wake-up
    struct timeval now;
    gettimeofday(&now, NULL);
    const int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    switch(esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            ESP_LOGI(APP_TAG, "Wake up from timer. Time spent in deep sleep: %dms", sleep_time_ms);
            break;
        }
        case ESP_SLEEP_WAKEUP_ULP: {
            ESP_LOGI(APP_TAG, "ESP_SLEEP_WAKEUP_ULP");
            break;
        }
        case ESP_SLEEP_WAKEUP_UNDEFINED: {
            ESP_LOGI(APP_TAG, "Wakeup was not caused by deep sleep");
            break;
        }
        default:
            ESP_LOGI(APP_TAG, "Not a deep sleep reset");
    }

    // open the "broadcast" key-value pair from the "state" namespace in NVS
    uint8_t broadcast = 0; // value will default to 0, if not set yet in NVS

    nvs_handle_t nvs_h;
    ret = nvs_open("state", NVS_READWRITE, &nvs_h);
    if(ret != ESP_OK) {
        ESP_LOGE(APP_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    } else {
        // Read
        ESP_LOGI(APP_TAG, "Reading broadcast state from NVS");
        ret = nvs_get_u8(nvs_h, "broadcast", &broadcast);
        switch (ret) {
            case ESP_OK:
                ESP_LOGI(APP_TAG, "Broadcast = %d", broadcast);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGI(APP_TAG, "The value is not initialized yet!");
                break;
            default :
                ESP_LOGE(APP_TAG, "Error (%s) reading!", esp_err_to_name(ret));
        }

        // Write
        ESP_LOGI(APP_TAG, "Updating broadcast state in NVS ... ");
        broadcast++;
        ret = nvs_set_u8(nvs_h, "broadcast", broadcast);
        if(ret != ESP_OK) {
            ESP_LOGE(APP_TAG, "Error (%s) setting broadcast state", esp_err_to_name(ret));
        }

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        ESP_LOGI(APP_TAG, "Committing updates in NVS ... ");
        ret = nvs_commit(nvs_h);
        if(ret != ESP_OK) {
            ESP_LOGE(APP_TAG, "Error (%s) commiting broadcast state", esp_err_to_name(ret));
        } else {
            ESP_LOGI(APP_TAG, "Committed NVS Updates");
        }
        // Close
        nvs_close(nvs_h);
    }
// #if CONFIG_PM_ENABLE
//     // Configure dynamic frequency scaling:
//     // maximum and minimum frequencies are set in sdkconfig,
//     // automatic light sleep is enabled if tickless idle support is enabled.
// #if CONFIG_IDF_TARGET_ESP32
//     esp_pm_config_esp32_t pm_config = {
// #elif CONFIG_IDF_TARGET_ESP32S2
//     esp_pm_config_esp32s2_t pm_config = {
// #endif
//             .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ,
//             .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ,
// #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
//             .light_sleep_enable = true
// #endif
//     };
//     ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
// #endif // CONFIG_PM_ENABLE

    // setup sensor to perform readings
    xTaskCreate(sensor_task, "sensor_task", configMINIMAL_STACK_SIZE * 8, (void*)&sensor, 5, NULL);

    // setup broadcasting method
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if(broadcast % 2 == 0) {
        xTaskCreate(http_task, "http_task", 8192, (void*)&sensor, 5, NULL);
    } else {
        xTaskCreate(ble_task, "ble_task", 8192*2, NULL, 5, NULL);
    }
#else
    xTaskCreate(http_task, "http_task", 8192, (void*)&sensor, 5, NULL);
#endif

    // setup watcher for sleep
    xTaskCreate(sleep_task, "sleep_task", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
