#ifndef SENSORS
#define SENSORS

#include <vector>
#include <cJSON.h>

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
static LDM::Camera camera(FRAMESIZE_VGA, PIXFORMAT_JPEG, 30, 1);
#endif

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

static cJSON * json_data = NULL;

#endif
