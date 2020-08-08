#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#if CONFIG_DHT11_SENSOR_ENABLED

#include <esp_camera.h>
#include <esp_http_server.h>

#ifdef CONFIG_CAMERA_MODEL_ESP_EYE

#define CAM_BOARD        "ESP-EYE"
#define CAM_PIN_PWDN     -1
#define CAM_PIN_RESET    -1
#define CAM_PIN_XCLK     4
#define CAM_PIN_SIOD     18
#define CAM_PIN_SIOC     23

#define CAM_PIN_D7       36
#define CAM_PIN_D6       37
#define CAM_PIN_D5       38
#define CAM_PIN_D4       39
#define CAM_PIN_D3       35
#define CAM_PIN_D2       14
#define CAM_PIN_D1       13
#define CAM_PIN_D0       34
#define CAM_PIN_VSYNC    5
#define CAM_PIN_HREF     27
#define CAM_PIN_PCLK     25

#endif

namespace LDM {
class Camera : public Sensor {
public:
    Camera();
    // float getHumidity(void);
    // float getTemperature(void);
    // void setHumidity(float humidity);
    // void setTemperature(float temperature);
    //
    // bool init(void);
    // bool deinit(void);
    // bool readSensor(void);
    // cJSON *buildJson(void);

// private:
//     float temperature;
//     float humidity;
//
//     static const gpio_num_t dht_gpio = static_cast<gpio_num_t>(DHT_GPIO);
//     static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
};
}
#endif

#endif
