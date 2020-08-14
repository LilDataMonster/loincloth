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

#if CONFIG_CAMERA_SENSOR_ENABLED
#include <ldm-camera.hpp>
static LDM::Camera sensor(FRAMESIZE_SVGA, PIXFORMAT_JPEG, 12, 1);
#endif

#if CONFIG_BME680_SENSOR_ENABLED
#include <bme680.hpp>
static LDM::BME680 sensor;
#endif

#include <nvs.hpp>
#include <sleep.hpp>

extern "C" {
void app_main(void);
}

// static RTC_DATA_ATTR struct timeval sleep_enter_time;

void app_main(void) {

    LDM::Sleep::getWakeupCause();
    // esp_sleep_source_t wakeup_cause = nvs.getWakeupCause();
    // nvs.getWakeupCause();

    // open the "broadcast" key-value pair from the "state" namespace in NVS
    uint8_t broadcast = 0; // value will default to 0, if not set yet in NVS

    // initialize nvs
    LDM::NVS nvs;
    nvs.openNamespace("state");
    // nvs.getKeyU8("broadcast", &broadcast);
    // broadcast++;
    // nvs.setKeyU8("broadcast", broadcast);
    nvs.commit();
    nvs.close();

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
