#ifndef __BLE_HPP__
#define __BLE_HPP__
#ifndef CONFIG_IDF_TARGET_ESP32S2

#include <esp_err.h>
#include <string>

#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>

#define PROFILE_NUM 1
// #define MANUFACTURER_DATA_LEN  17
#define MANUFACTURER_DATA_LEN  4

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    // uint16_t app_id;
    // uint16_t conn_id;
    // uint16_t service_handle;
    // esp_gatt_srvc_id_t service_id;
    // uint16_t char_handle;
    // esp_bt_uuid_t char_uuid;
    // esp_gatt_perm_t perm;
    // esp_gatt_char_prop_t property;
    // uint16_t descr_handle;
    // esp_bt_uuid_t descr_uuid;
};

namespace LDM {
class BLE {
public:
    BLE(std::string deviceName);
    esp_err_t init(void);
    esp_err_t deinit(void);
    esp_err_t setupCallback(void);
    esp_err_t updateValue(uint8_t humidity, uint8_t temperature);

private:
    static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
    static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
    static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

    static std::string device_name;
    static uint8_t manufacturer_data[MANUFACTURER_DATA_LEN];

    static uint8_t adv_service_uuid128[32] ;
    static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM];

    static esp_ble_adv_data_t adv_data;
    static esp_ble_adv_data_t scan_rsp_data;

    static uint8_t adv_config_done;
};
}
#endif
#endif
