// #ifdef BME680_SENSOR_ENABLED

#include <esp_log.h>
#include <cJSON.h>
#include <cstring>

//BME680 sensor
#include <bme680.h>

// project headers
#include <bme680.hpp>

#define BME680_TAG "BME680_SENSOR"
#define BME680_INFO(fmt, ...)   ESP_LOGI(BME680_TAG, fmt, ##__VA_ARGS__)
#define BME680_ERROR(fmt, ...)  ESP_LOGE(BME680_TAG, fmt, ##__VA_ARGS__)

LDM::BME680::BME680() : temperature(0), humidity(0), pressure(0), gas(0) {
    ESP_ERROR_CHECK(i2cdev_init());
};

bool LDM::BME680::init(void) {
    memset(&this->sensor, 0, sizeof(bme680_t));

    ESP_ERROR_CHECK(bme680_init_desc(&this->sensor, ADDR, i2c_port_num, sda_gpio, scl_gpio));
    //ESP_ERROR_CHECK(bme680_init_desc(&sensor, (gpio_num_t)ADDR, (gpio_num_t)PORT, (gpio_num_t)SDA_GPIO, (gpio_num_t)SCL_GPIO));

    // init the sensor
    ESP_ERROR_CHECK(bme680_init_sensor(&this->sensor));

    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. Pressure measurement is skipped.
    bme680_set_oversampling_rates(&this->sensor, BME680_OSR_4X, BME680_OSR_4X, BME680_OSR_4X);

    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(&this->sensor, BME680_IIR_SIZE_3);

    // Change the heater profile 0 to 200 degree Celcius for 100 ms.
    bme680_set_heater_profile(&this->sensor, 0, 200, 100);
    bme680_use_heater_profile(&this->sensor, 0);

    // Set ambient temperature to 10 degree Celsius
    bme680_set_ambient_temperature(&this->sensor, 10);

    // as long as sensor configuration isn't changed, duration is constant
    bme680_get_measurement_duration(&this->sensor, &this->duration);

    BME680_INFO("BME680 Initialized for I2C GPIO %d, SDA GPIO %d, SCL GPIO %d", (int)i2c_port_num, (int)sda_gpio, (int)scl_gpio);

    return true;
};

bool LDM::BME680::deinit(void) {
    return true;
};

bool LDM::BME680::readSensor(void) {
    // trigger the sensor to start one TPHG measurement cycle
    if(bme680_force_measurement(&sensor) == ESP_OK) {
        bme680_values_float_t values;

        // passive waiting until measurement results are available
        vTaskDelay(this->duration);

        // get the results and do something with them
        if(bme680_get_results_float(&sensor, &values) == ESP_OK) {
            BME680_INFO("BME680 Sensor: %.2f °C, %.2f %%, %.2f hPa, %.2f Ohm\n",
                values.temperature, values.humidity, values.pressure, values.gas_resistance);
            this->setHumidity(values.humidity);
            this->setTemperature(values.temperature);
            this->setPressure(values.pressure);
            this->setGas(values.gas_resistance);
            BME680_INFO("Humidity: %.2f, Temp: %.2fC, %.2f hPa, %.2f Ohm\n",
                this->getHumidity(), this->getTemperature(), this->getPressure(), this->getGas());
            return true;
        } else {
            BME680_INFO("Could not read data from BME680 sensor");
            return false;
        }
    }

    return true;
};

cJSON* LDM::BME680::buildJson(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", this->temperature);
    cJSON_AddNumberToObject(root, "humidity", this->humidity);
    cJSON_AddNumberToObject(root, "pressure", this->pressure);
    cJSON_AddNumberToObject(root, "gas", this->gas);
    return root;
};

float LDM::BME680::getHumidity(void) {
    return this->humidity;
};

float LDM::BME680::getTemperature(void) {
    return this->temperature;
};

float LDM::BME680::getPressure(void) {
    return this->pressure;
};

float LDM::BME680::getGas(void) {
    return this->gas;
};

void LDM::BME680::setHumidity(float humidity) {
    this->humidity = humidity;
};

void LDM::BME680::setTemperature(float temperature) {
    this->temperature = temperature;
};

void LDM::BME680::setPressure(float pressure) {
    this->pressure = pressure;
};

void LDM::BME680::setGas(float gas) {
    this->gas = gas;
};

// #endif
