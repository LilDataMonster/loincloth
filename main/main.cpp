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

    // sensors.at(0)->init();
    LDM::System system;
    cJSON *system_json = system.buildJson();

    cJSON *message = cJSON_CreateObject();
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

    xTaskCreate(http_task, "http_task", 8192, (void*)output, 5, NULL);

    // // setup watcher for sleep
    // xTaskCreate(sleep_task, "sleep_task", configMINIMAL_STACK_SIZE, (void*)&sensors, 5, NULL);
}
