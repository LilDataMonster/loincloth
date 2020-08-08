#include <cstdlib>
#include <string>
#include <cstring>

#include <esp_log.h>

#ifndef CONFIG_IDF_TARGET_ESP32S2
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <esp_bt_defs.h>
#include <esp_bt_main.h>

// project headers
#include <ble.hpp>

#define PROFILE_A_APP_ID 0

#define BLE_TAG "BLE"
#define GATTS_TAG "GATTS"

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

uint8_t LDM::BLE::manufacturer_data[MANUFACTURER_DATA_LEN] =  {
    'T', 'H', 0x00, 0x00,// 0x01, 0x02, 0x03, 0x04,
};

// define static member variables
std::string LDM::BLE::device_name = "Device";
uint8_t LDM::BLE::adv_config_done = 0;
struct gatts_profile_inst LDM::BLE::gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

esp_ble_adv_data_t LDM::BLE::adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    // .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    // .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0xFF,
    // .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .manufacturer_len = MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = &manufacturer_data[0],
    .service_data_len = MANUFACTURER_DATA_LEN,
    .p_service_data = &manufacturer_data[0],
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_data_t LDM::BLE::scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0xFF,
    // .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    // .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .manufacturer_len = MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = &manufacturer_data[0],
    .service_data_len = MANUFACTURER_DATA_LEN,
    .p_service_data = &manufacturer_data[0],
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

uint8_t LDM::BLE::adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xCC, 0xDD, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAA, 0x22, 0x00, 0x00,
};

// define BLE functions
LDM::BLE::BLE(std::string deviceName) {
    LDM::BLE::device_name = deviceName;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // setup default bluetooth configuration
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_err_t status = esp_bt_controller_init(&bt_cfg);
    if(status) {
        ESP_LOGE(BLE_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(status));
        return;
    }
    status |= esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if(status) {
        ESP_LOGE(BLE_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(status));
        return;
    }
}

esp_err_t LDM::BLE::init(void) {

    esp_err_t status = esp_bluedroid_init();
    if(status) {
        ESP_LOGE(BLE_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(status));
        return status;
    }
    status |= esp_bluedroid_enable();
    if(status) {
        ESP_LOGE(BLE_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(status));
        return status;
    }
    return status;
}

esp_err_t LDM::BLE::deinit(void) {

    esp_err_t status = esp_bluedroid_disable();
    if(status) {
        ESP_LOGE(BLE_TAG, "%s disable bluetooth failed: %s\n", __func__, esp_err_to_name(status));
        return status;
    }
    status |= esp_bluedroid_deinit();
    if(status) {
        ESP_LOGE(BLE_TAG, "%s deinit bluetooth failed: %s\n", __func__, esp_err_to_name(status));
        return status;
    }
    return status;
}

esp_err_t LDM::BLE::setupCallback(void) {

    esp_err_t status = esp_ble_gatts_register_callback(gatts_event_handler);
    if(status){
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", status);
        return status;
    }
    status |= esp_ble_gap_register_callback(gap_event_handler);
    if(status){
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", status);
        return status;
    }

    status |= esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if(status){
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", status);
        return status;
    }
    return status;
}

esp_err_t LDM::BLE::updateValue(uint8_t humidity, uint8_t temperature) {
    LDM::BLE::manufacturer_data[2] = temperature;
    LDM::BLE::manufacturer_data[3] = humidity;

    // refresh advertising data
    esp_err_t status = esp_ble_gap_config_adv_data(&adv_data);
    if(status){
        ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", status);
    }
    // adv_config_done |= adv_config_flag;
    //config scan response data
    status = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if(status){
        ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", status);
    }
    return status;
}

void LDM::BLE::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    esp_ble_adv_params_t adv_params = {
        .adv_int_min        = 0x20,
        .adv_int_max        = 0x40,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        //.peer_addr            =
        //.peer_addr_type       =
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if(adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if(adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if(param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if(param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
            param->update_conn_params.status,
            param->update_conn_params.min_int,
            param->update_conn_params.max_int,
            param->update_conn_params.conn_int,
            param->update_conn_params.latency,
            param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void LDM::BLE::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    /* If event is register event, store the gatts_if for each profile */
    if(event == ESP_GATTS_REG_EVT) {
        if(param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                param->reg.app_id,
                param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for(idx = 0; idx < PROFILE_NUM; idx++) {
            if(gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if(gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


void LDM::BLE::gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT: {
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        // gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        // gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        // gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        // gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        ESP_LOGI(GATTS_TAG, "Setting Device Name: %s", device_name.c_str());
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(device_name.c_str());
        if(set_dev_name_ret){
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if(ret){
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if(ret){
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }

        // esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    }
    case ESP_GATTS_READ_EVT:
    case ESP_GATTS_WRITE_EVT:
    case ESP_GATTS_EXEC_WRITE_EVT:
    case ESP_GATTS_MTU_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_CREATE_EVT:
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
    case ESP_GATTS_ADD_CHAR_EVT:
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    case ESP_GATTS_DELETE_EVT:
    case ESP_GATTS_START_EVT:
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_CONNECT_EVT:
    case ESP_GATTS_DISCONNECT_EVT:
    case ESP_GATTS_CONF_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}
#endif
