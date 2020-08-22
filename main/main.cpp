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

enum BoardMode { setup, sleep };
static BoardMode mode = setup;

#if CONFIG_DHT_SENSOR_ENABLED
#include <dht.hpp>
static LDM::DHT dht;
#endif

#if CONFIG_BME680_SENSOR_ENABLED
#include <bme680.hpp>
static LDM::BME680 bme680;
#endif

#if CONFIG_CAMERA_SENSOR_ENABLED
#include <camera.hpp>
static LDM::Camera camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 12, 1);
#endif

#include <nvs.hpp>
#include <sleep.hpp>
#include <system.hpp>
#include <server.hpp>

extern "C" {
void app_main(void);
}

// initialize vector of sensors
static std::vector<LDM::Sensor*> sensors {
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

//     std::vector<LDM::Sensor*> sensors;
// #if CONFIG_DHT_SENSOR_ENABLED
//     sensors.push_back(&dht);
// #endif
// #if CONFIG_BME680_SENSOR_ENABLED
//     sensors.push_back(&bme680);
// #endif
// #if CONFIG_CAMERA_SENSOR_ENABLED
//     sensors.push_back(&camera);
// #endif

    // std::vector<uint8_t> sensors;
    // sensors.push_back(123);

//     LDM::Sensor *sensors[] = {
// #if CONFIG_DHT_SENSOR_ENABLED
//         &dht,
// #endif
// #if CONFIG_BME680_SENSOR_ENABLED
//         &bme680,
// #endif
// #if CONFIG_CAMERA_SENSOR_ENABLED
//         &camera,
// #endif
//     };

    // sensors.at(0)->init();
    LDM::System system;

    // const size_t num_sensors = (sizeof(sensors)/sizeof(*sensors));
    // ESP_LOGI(APP_TAG, "Number of Sensors: %u", num_sensors);
    // ESP_LOGI(APP_TAG, "Number of Sensors: %u", sensors.size());

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

    // for(auto const& sensor : sensors) {
    //     sensor->init();
    //     sensor->readSensor();
    // }

    // cJSON *message = cJSON_CreateObject();
    // for(auto const& sensor : sensors) {
    //     cJSON *json_sensor = sensor->buildJson();
    //     // cJSON_AddItemToObject(message, sensor->getSensorName(), json_sensor);
    //
    //     // printf("%s", sensor->getSensorName());
    //
    //     // char* output = cJSON_Print(json_sensor);
    //     // if(output != NULL) {
    //     //     printf("%s", output);
    //     // } else {
    //     //     printf("cJSON_Print output is NULL!\n");
    //     // }
    //     cJSON_AddItemToObject(message, "sensor", json_sensor);
    // }

    cJSON *message = cJSON_CreateObject();

    cJSON *system_json = system.buildJson();
    cJSON_AddItemToObject(message, APP_TAG, system_json);

    dht.init();
    dht.readSensor();
    cJSON *dht_json = dht.buildJson();
    cJSON_AddItemToObject(message, dht.getSensorName(), dht_json);

    camera.init();
    camera.readSensor();
    cJSON *camera_json = camera.buildJson();
    cJSON_AddItemToObject(message, camera.getSensorName(), camera_json);

    char* output = cJSON_Print(message);

    if(mode == BoardMode::setup) {
        ESP_LOGI(APP_TAG, "Board in setup mode");
    }
    
    // printf("%s", output);

    // // setup sensor to perform readings
    // xTaskCreate(sensor_task, "sensor_task", configMINIMAL_STACK_SIZE * 8, (void*)&sensors, 5, NULL);

//     // setup broadcasting method
// // #ifndef CONFIG_IDF_TARGET_ESP32S2
// //     if(broadcast % 2 == 0) {
// //         xTaskCreate(http_task, "http_task", 8192, (void*)&sensor, 5, NULL);
// //     } else {
// //         xTaskCreate(ble_task, "ble_task", 8192*2, NULL, 5, NULL);
// //     }
// // #else
    // xTaskCreate(http_task, "http_task", 5*8192, (void*)&sensors, 5, NULL);
    xTaskCreate(http_task, "http_task", 8192, (void*)output, 5, NULL);
    // xTaskCreate(http_task, "http_task", 8192, (void*)&sensors, 5, NULL);
// // #endif

    // // setup watcher for sleep
    // xTaskCreate(sleep_task, "sleep_task", configMINIMAL_STACK_SIZE, (void*)&sensors, 5, NULL);
}
