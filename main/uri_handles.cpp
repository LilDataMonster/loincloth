#include <cJSON.h>
#include <esp_log.h>
#include <camera.hpp>
#include <sensors.hpp>
#include <uri_handles.hpp>

#define TAG "URI_HANDLE"

// #define SCRATCH_BUFSIZE (1024)
// typedef struct rest_server_context {
//     // char base_path[ESP_VFS_PATH_MAX + 1];
//     char scratch[SCRATCH_BUFSIZE];
// } rest_server_context_t;

rest_server_context_t *rest_context = (rest_server_context_t*)calloc(1, sizeof(rest_server_context_t));

typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

// GET URI
httpd_uri_t uri_get = {
    .uri      = "/info",
    .method   = HTTP_GET,
    .handler  = get_handler,
    .user_ctx = NULL
};

esp_err_t get_handler(httpd_req_t *req) {
    /* Send a simple response */
    const char resp[] = "Woot";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


// POST URI
httpd_uri_t uri_post = {
    .uri      = "/post",
    .method   = HTTP_POST,
    .handler  = post_handler,
    .user_ctx = NULL
};

esp_err_t post_handler(httpd_req_t *req) {
    /* Send a simple response */
    const char resp[] = "Woot";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
};

// Data Post URI
esp_err_t data_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "In DATA POST fn");
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    int red = 0;
    int green = 0;
    int blue = 0;

    cJSON *root = cJSON_Parse(buf);
    cJSON *red_json = cJSON_GetObjectItem(root, "red");
    if(red_json) {
        red = red_json->valueint;
    } else {
        ESP_LOGI(TAG, "Red value not found");
    }
    cJSON *green_json = cJSON_GetObjectItem(root, "green");
    if(green_json) {
        green = green_json->valueint;
    } else {
        ESP_LOGI(TAG, "Green value not found");
    }
    cJSON *blue_json = cJSON_GetObjectItem(root, "blue");
    if(blue_json) {
        blue = blue_json->valueint;
    } else {
        ESP_LOGI(TAG, "Blue value not found");
    }
    ESP_LOGI(TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

httpd_uri_t uri_data = {
   .uri      = "/data",
   .method   = HTTP_POST,
   .handler  = data_post_handler,
   .user_ctx = rest_context
};

// Camera URI
static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len) {
    jpg_chunking_t *j = (jpg_chunking_t *) arg;
    if(!index) {
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
        return 0;
    }
    j->len += len;
    return len;
}

esp_err_t jpg_get_image_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    int64_t fr_start = esp_timer_get_time();

    // fb = esp_camera_fb_get();
    // if (!fb) {
    //     ESP_LOGE(TAG, "Camera capture failed");
    //     httpd_resp_send_500(req);
    //     return ESP_FAIL;
    // }
    camera.releaseData();
    res = camera.readSensor();
    if(res != ESP_OK) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }

    fb_len = camera.getJpgBufferLength();
    res = httpd_resp_send(req, (const char *)camera.getJpgBuffer(), camera.getJpgBufferLength());

    // fb = camera.getFrameBuffer();
    // jpg_chunking_t jchunk = {req, 0};
    // res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
    // httpd_resp_send_chunk(req, NULL, 0);
    // fb_len = jchunk.len;

    // if(res == ESP_OK){
    //     if(fb->format == PIXFORMAT_JPEG){
    //         fb_len = fb->len;
    //         res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    //     } else {
    //         jpg_chunking_t jchunk = {req, 0};
    //         res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
    //         httpd_resp_send_chunk(req, NULL, 0);
    //         fb_len = jchunk.len;
    //     }
    // }
    // esp_camera_fb_return(fb);
    int64_t fr_end = esp_timer_get_time();
    ESP_LOGI(TAG, "JPG: %uKB %ums", (uint32_t)(fb_len/1024), (uint32_t)((fr_end - fr_start)/1000));
    return res;
}

httpd_uri_t uri_get_camera = {
   .uri      = "/camera",
   .method   = HTTP_GET,
   .handler  = jpg_get_image_handler,
   .user_ctx = NULL
};

#define GET_INT_VAL_TYPE(_json, _root, _key, _fn, _type)         \
      cJSON * _json = cJSON_GetObjectItem(_root, #_key);    \
      if(_json) {                                           \
          s->_fn(s, (_type)_json->valueint);                \
          ESP_LOGI(TAG, #_key" value set: %d", _json->valueint);           \
      } else {                                              \
          ESP_LOGI(TAG, #_key" value not found");           \
      }

#define GET_INT_VAL(_json, _root, _key, _fn) \
      GET_INT_VAL_TYPE(_json, _root, _key, _fn, int)

esp_err_t camera_post_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    sensor_t *s = camera.getSensor();

    cJSON *root = cJSON_Parse(buf);
    // framesize [0-10]
    GET_INT_VAL_TYPE(framesize_json, root, framesize, set_framesize, framesize_t);
    GET_INT_VAL(quality_json, root, quality, set_quality);
    GET_INT_VAL(contrast_json, root, contrast, set_contrast);
    GET_INT_VAL(brightness_json, root, brightness, set_brightness);
    GET_INT_VAL(saturation_json, root, saturation, set_saturation);
    GET_INT_VAL_TYPE(gainceiling_json, root, gainceiling, set_gainceiling, gainceiling_t);
    GET_INT_VAL(colorbar_json, root, colorbar, set_colorbar);
    GET_INT_VAL(whitebal_json, root, whitebal, set_whitebal);
    GET_INT_VAL(gain_ctrl_json, root, gain_ctrl, set_gain_ctrl);
    GET_INT_VAL(exposure_ctrl_json, root, exposure_ctrl, set_exposure_ctrl);
    GET_INT_VAL(hmirror_json, root, hmirror, set_hmirror);
    GET_INT_VAL(vflip_json, root, vflip, set_vflip);
    GET_INT_VAL(awb_gain_json, root, awb_gain, set_awb_gain);
    GET_INT_VAL(agc_gain_json, root, agc_gain, set_agc_gain);
    GET_INT_VAL(aec_value_json, root, aec_value, set_aec_value);
    GET_INT_VAL(aec2_json, root, aec2, set_aec2);
    GET_INT_VAL(dcw_json, root, dcw, set_dcw);
    GET_INT_VAL(bpc_json, root, bpc, set_bpc);
    GET_INT_VAL(wpc_json, root, wpc, set_wpc);
    GET_INT_VAL(raw_gma_json, root, raw_gma, set_raw_gma);
    GET_INT_VAL(lenc_json, root, lenc, set_lenc);
    GET_INT_VAL(special_effect_json, root, special_effect, set_special_effect);
    GET_INT_VAL(wb_mode_json, root, wb_mode, set_wb_mode);
    GET_INT_VAL(ae_level_json, root, ae_level, set_ae_level);

    cJSON *duty_json = cJSON_GetObjectItem(root, "duty");
    if(duty_json) {
        led_duty = duty_json->valueint;
    } else {
        ESP_LOGI(TAG, "Duty value not found");
    }
    httpd_resp_sendstr(req, "Camera Parameters Updated successfully");
    return res;
};

httpd_uri_t uri_post_camera = {
   .uri      = "/camera",
   .method   = HTTP_POST,
   .handler  = camera_post_handler,
   .user_ctx = rest_context
};

// Stream URI
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
esp_err_t jpg_get_stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        return res;
    }

    while(true){
        camera.releaseData();
        res = camera.readSensor();
        if(res != ESP_OK) {
            ESP_LOGE(TAG, "Camera capture failed");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        _jpg_buf_len = camera.getJpgBufferLength();
        _jpg_buf = camera.getJpgBuffer();
        // if(fb->format != PIXFORMAT_JPEG){
        //     bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        //     if(!jpeg_converted){
        //         ESP_LOGE(TAG, "JPEG compression failed");
        //         esp_camera_fb_return(fb);
        //         res = ESP_FAIL;
        //     }
        // } else {
        //     _jpg_buf_len = fb->len;
        //     _jpg_buf = fb->buf;
        // }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        // if(fb->format != PIXFORMAT_JPEG){
        //     free(_jpg_buf);
        // }
        // esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}

httpd_uri_t uri_get_stream = {
   .uri      = "/stream",
   .method   = HTTP_GET,
   .handler  = jpg_get_stream_handler,
   .user_ctx = NULL
};

// led Post URI
esp_err_t led_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "In DATA POST fn");
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    cJSON *duty_json = cJSON_GetObjectItem(root, "duty");
    if(duty_json) {
        led_duty = duty_json->valueint;
    } else {
        ESP_LOGI(TAG, "Duty value not found");
    }
    cJSON *fade_json = cJSON_GetObjectItem(root, "fade");
    if(fade_json) {
        led_fade_time = fade_json->valueint;
    } else {
        ESP_LOGI(TAG, "Fade value not found");
    }
    cJSON *on_json = cJSON_GetObjectItem(root, "on");
    if(fade_json) {
        led_on = on_json->valueint;
    } else {
        ESP_LOGI(TAG, "On value not found");
    }
    ESP_LOGI(TAG, "LED control: duty = %d, fade = %d, on = %d", led_duty, led_fade_time, led_on);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "LED values updated successfully");
    return ESP_OK;
}

httpd_uri_t uri_post_led = {
   .uri      = "/led",
   .method   = HTTP_POST,
   .handler  = led_post_handler,
   .user_ctx = rest_context
};
