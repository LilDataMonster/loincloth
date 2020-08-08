#include <esp_log.h>
#include <cJSON.h>

// DHT sensor
#include <dht.h>

// project headers
#include <dht.hpp>

#define DHT_TAG "DHT_SENSOR"
#define DHT_INFO(fmt, ...)   ESP_LOGI(DHT_TAG, fmt, ##__VA_ARGS__)
#define DHT_ERROR(fmt, ...)  ESP_LOGE(DHT_TAG, fmt, ##__VA_ARGS__)

LDM::DHT::DHT() : temperature(0), humidity(0) {

};

bool LDM::DHT::init(void) {
    // DHT sensors that come mounted on a PCB generally have
    // pull-up resistors on the data pin.  It is recommended
    // to provide an external pull-up resistor otherwise...
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);

    DHT_INFO("DHT Initialized for GPIO %d", (int)dht_gpio);
    return true;
};

bool LDM::DHT::deinit(void) {
    return true;
};

bool LDM::DHT::readSensor(void) {
    int16_t temperature, humidity;
    if(dht_read_data(this->sensor_type, this->dht_gpio, &humidity, &temperature) == ESP_OK) {
        this->setHumidity(humidity / 10.f);
        this->setTemperature(temperature / 10.f);
        DHT_INFO("Humidity: %.2f, Temp: %.2fC", this->getHumidity(), this->getTemperature());
        return true;
    }
    else {
        DHT_INFO("Could not read data from sensor");
        return false;
    }
};

cJSON* LDM::DHT::buildJson(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "temperature", this->temperature);
    cJSON_AddNumberToObject(root, "humidity", this->humidity);

    return root;
};

float LDM::DHT::getHumidity(void) {
    return this->humidity;
};

float LDM::DHT::getTemperature(void) {
    return this->temperature;
};

void LDM::DHT::setHumidity(float humidity) {
    this->humidity = humidity;
};

void LDM::DHT::setTemperature(float temperature) {
    this->temperature = temperature;
};
