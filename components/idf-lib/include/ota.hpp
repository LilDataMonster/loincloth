#ifndef __OTA_HPP__
#define __OTA_HPP__
#include <esp_https_ota.h>
#include <esp_http_client.h>

namespace LDM {
class OTA {
public:
    OTA(char* URL);
    esp_err_t checkUpdates(bool restartOnSuccess=true);

private:
    esp_http_client_config_t config;
};
}
#endif
