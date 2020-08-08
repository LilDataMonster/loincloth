#ifndef __SENSOR_HPP__
#define __SENSOR_HPP__

#include <cJSON.h>

namespace LDM {
class Sensor {
public:
    // sensors must implement the following functions
    virtual bool init(void) = 0;
    virtual bool deinit(void) = 0;
    virtual bool readSensor(void) = 0;
    virtual cJSON *buildJson(void) = 0;
};
}

#endif
