#ifndef URI_HANDLES
#define URI_HANDLES

#include <esp_http_server.h>

/* Our URI handler function to be called during GET /uri request */
esp_err_t get_handler(httpd_req_t *req);

/* URI handler structure for GET /uri */
extern httpd_uri_t uri_get;

esp_err_t post_handler(httpd_req_t *req);
extern httpd_uri_t uri_post;

esp_err_t data_post_handler(httpd_req_t *req);
extern httpd_uri_t uri_data;

esp_err_t jpg_get_image_handler(httpd_req_t *req);
extern httpd_uri_t uri_get_camera;

esp_err_t camera_post_handler(httpd_req_t *req);
extern httpd_uri_t uri_post_camera;

esp_err_t camera_options_handler(httpd_req_t *req);
extern httpd_uri_t uri_options_camera;

esp_err_t jpg_get_stream_handler(httpd_req_t *req);
extern httpd_uri_t uri_get_stream;

esp_err_t led_post_handler(httpd_req_t *req);
extern httpd_uri_t uri_post_led;

#endif
