#include <uri_handles.hpp>

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri      = "/info",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req) {
    /* Send a simple response */
    const char resp[] = "Woot";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


/* URI handler structure for GET /uri */
httpd_uri_t uri_post = {
    .uri      = "/post",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

/* Our URI handler function to be called during GET /uri request */
esp_err_t post_handler(httpd_req_t *req) {
    /* Send a simple response */
    const char resp[] = "Woot";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
