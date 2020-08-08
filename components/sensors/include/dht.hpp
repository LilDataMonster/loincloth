#ifndef __DHT_HPP__
#define __DHT_HPP__

#include <cJSON.h>
#include <sensor.hpp>

// DHT sensor
#include <dht.h>

#ifdef CONFIG_DHT_SENSOR_ENABLED
#define DHT_GPIO CONFIG_DHT_GPIO

#ifdef CONFIG_DHT_TYPE_DHT11
#define DHT_TYPE_DHT11 DHT_TYPE_DHT11
#elif CONFIG_DHT_TYPE_DHT22
#define DHT_TYPE_DHT11 DHT_TYPE_DHT22
#else
#define DHT_TYPE_DHT11 DHT_TYPE_DHT11
#endif

#else
#define DHT_GPIO 4
#define DHT_TYPE_DHT11 DHT_TYPE_DHT11
#endif

namespace LDM {
class DHT : public Sensor {
public:
    DHT();
    float getHumidity(void);
    float getTemperature(void);
    void setHumidity(float humidity);
    void setTemperature(float temperature);

    bool init(void);
    bool deinit(void);
    bool readSensor(void);
    cJSON *buildJson(void);

private:
    float temperature;
    float humidity;

    static const gpio_num_t dht_gpio = static_cast<gpio_num_t>(DHT_GPIO);
    static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
};
}
#endif
