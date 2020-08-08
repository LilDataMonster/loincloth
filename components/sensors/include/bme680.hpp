#ifndef __BME680_HPP__
#define __BME680_HPP__

#include <cJSON.h>
#include <sensor.hpp>

//BME680 sensor
#include <bme680.h>

#ifdef CONFIG_BME680_SENSOR_ENABLED
#define I2C_SCL CONFIG_I2C_SCL
#define I2C_SDA CONFIG_I2C_SDA
#define I2C_PORT CONFIG_I2C_PORT
#else
#define I2C_SCL 19
#define I2C_SDA 18
#define I2C_PORT 0
#endif

#define ADDR BME680_I2C_ADDR_1

namespace LDM {
class BME680 : public Sensor{
public:
    BME680();
    float getHumidity(void);
    float getTemperature(void);
    float getPressure(void);
    float getGas(void);
    void setHumidity(float humidity);
    void setTemperature(float temperature);
    void setPressure(float pressure);
    void setGas(float gas);

    bool init(void);
    bool deinit(void);
    bool readSensor(void);
    cJSON *buildJson(void);

private:
    float temperature;
    float humidity;
    int pressure;
    int gas;

    bme680_t sensor;
    uint32_t duration;

    static const gpio_num_t scl_gpio = static_cast<gpio_num_t>(I2C_SCL);
    static const gpio_num_t sda_gpio = static_cast<gpio_num_t>(I2C_SDA);
    static const gpio_num_t i2c_port_num = static_cast<gpio_num_t>(I2C_PORT);
};
}
#endif
