#include <cstdlib>
#include <esp_log.h>
#include <esp_event.h>

// JSON formatting
#include <cJSON.h>

// HTTP request
#include <esp_tls.h>
#include <esp_http_client.h>

// project headers
#include <http.hpp>

#define HTTP_TAG "HTTP"

LDM::HTTP::HTTP(char* URL) {
    // create http client
    this->config = {
        .url = URL,
        .user_data = this->response_buffer,
    };
    this->client = esp_http_client_init(&this->config);
}

esp_err_t LDM::HTTP::postJSON(cJSON *message) {
    ESP_LOGI(HTTP_TAG, "Running post_json");
    esp_err_t err = ESP_OK;

    if(message != NULL){
        char *post_data = cJSON_Print(message);

        if(post_data != NULL){
            ESP_LOGI(HTTP_TAG, "Sending JSON Message: %s", post_data);

            esp_http_client_set_method(this->client, HTTP_METHOD_POST);
            esp_http_client_set_header(this->client, "Content-Type", "application/json");
            esp_http_client_set_post_field(this->client, post_data, strlen(post_data));

            // post JSON message
            err = esp_http_client_perform(this->client);
            if (err == ESP_OK) {
                ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %d",
                        esp_http_client_get_status_code(this->client),
                        esp_http_client_get_content_length(this->client));
            } else {
                ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
            }
            cJSON_free((void*)post_data);
            post_data = NULL;
        }
    }
    return err;
}
