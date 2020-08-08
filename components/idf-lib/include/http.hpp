#ifndef __HTTP_HPP__
#define __HTTP_HPP__

// HTTP request
#include <esp_tls.h>
#include <esp_http_client.h>

// JSON formatting
#include <cJSON.h>

#define MAX_HTTP_OUTPUT_BUFFER 2048

namespace LDM {
class HTTP {
public:
    HTTP(char* URL);
    esp_http_client_handle_t getClient(void) {
        return this->client;
    }
    esp_err_t postJSON(cJSON *message);
    esp_err_t deinit(void) {
        return esp_http_client_cleanup(this->client);
    }

private:
    char response_buffer[MAX_HTTP_OUTPUT_BUFFER];
    esp_http_client_config_t config;
    esp_http_client_handle_t client;
};
}
#endif
