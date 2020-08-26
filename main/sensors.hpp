#ifndef SENSORS
#define SENSORS

#include <vector>
#include <cJSON.h>
#include <ble.hpp>
#include <server.hpp>
#include <system.hpp>


// probably should avoid to use extern
#if CONFIG_DHT_SENSOR_ENABLED
#include <dht.hpp>
extern LDM::DHT dht;
#endif

#if CONFIG_BME680_SENSOR_ENABLED
#include <bme680.hpp>
extern LDM::BME680 bme680;
#endif

#if CONFIG_CAMERA_SENSOR_ENABLED
#include <camera.hpp>
extern LDM::Camera camera;
#endif

// initialize vector of sensors
extern std::vector<LDM::Sensor*> sensors;

extern cJSON * json_data;
extern LDM::BLE *g_ble;
extern LDM::Server *g_http_server;
extern LDM::System *g_system;

extern uint16_t led_fade_time;
extern uint16_t led_duty;
extern int32_t led_on;

#endif
