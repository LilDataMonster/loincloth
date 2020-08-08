#ifndef __WIFI_HPP__
#define __WIFI_HPP__

#include <freertos/event_groups.h>
#include <esp_event.h>
#include <esp_err.h>

extern "C" {

namespace LDM {
class WiFi {
public:
    WiFi() {};
    esp_err_t init_sta(void);
    esp_err_t deinit_sta(void);

private:
    static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};
}
}

#endif
