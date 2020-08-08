#include <esp_https_ota.h>
#include <esp_http_client.h>
#include <esp_log.h>

#include <ota.hpp>

#define TAG "OTA"

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

LDM::OTA::OTA(char* URL) {
    ESP_LOGI(TAG, "Initializing OTA Parameters");

    // create http client
    this->config = {
        .url = URL,
        .cert_pem = (char *)server_cert_pem_start,
    };

#ifdef CONFIG_SKIP_COMMON_NAME_CHECK
    config.skip_cert_common_name_check = true;
#endif
}

esp_err_t LDM::OTA::checkUpdates(bool restartOnSuccess) {
    esp_err_t ret = esp_https_ota(&config);
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "Firmware upgrade success, restart to apply update");
        if(restartOnSuccess) {
            esp_restart();
        }
    } else {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }
    return ret;
}
