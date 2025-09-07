#include "ble_provisioning.h"

#include <string>
#include <cstring>

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gatts_api.h>
#include <esp_gap_ble_api.h>

#include <wifi_configuration_ap.h>

static const char* TAG = "BleProvisioning";

namespace {

static uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

// 简单的 GATT UUID（16 位自定义）
static const uint16_t kServiceUuid       = 0xFFF0;
static const uint16_t kWriteCharUuid     = 0xFFF1;

static uint16_t gatt_handle_table[3];
static esp_gatt_if_t g_gatts_if = ESP_GATT_IF_NONE;
static uint16_t g_conn_id = 0xFFFF;
static bool g_started = false;
static esp_ble_adv_params_t s_adv_params = {};
static std::string g_payload_buffer;

static esp_gatts_attr_db_t gatt_db[] = {
	// Service Declaration
	[0] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t*)&primary_service_uuid, ESP_GATT_PERM_READ,
		sizeof(uint16_t), sizeof(kServiceUuid), (uint8_t*)&kServiceUuid}},
	// Write Characteristic Declaration
	[1] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t*)&character_declaration_uuid, ESP_GATT_PERM_READ,
		1, 1, (uint8_t*)&char_prop_write}},
	// Write Characteristic Value
	[2] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t*)&kWriteCharUuid, ESP_GATT_PERM_WRITE,
		256, 0, nullptr}},
};

}

static void OnReceivedCredentials(const std::string& payload) {
	// 期望格式："SSID\nPASSWORD"
	size_t pos = payload.find('\n');
	if (pos == std::string::npos) {
		ESP_LOGE(TAG, "Invalid payload, missing newline");
		return;
	}
	std::string ssid = payload.substr(0, pos);
	std::string password = payload.substr(pos + 1);
	ESP_LOGI(TAG, "Received SSID=%s, PASSWORD len=%u", ssid.c_str(), (unsigned)password.size());

	auto& wifi_ap = WifiConfigurationAp::GetInstance();
	if (wifi_ap.ConnectToWifi(ssid, password)) {
		wifi_ap.Save(ssid, password);
		ESP_LOGI(TAG, "Credentials saved, rebooting...");
		esp_restart();
	} else {
		ESP_LOGE(TAG, "Connect failed with provided credentials");
	}
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t* param) {
	(void)gatts_if;
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		ESP_LOGI(TAG, "GATTS REG EVT");
		esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, 3, 0);
		break;
	}
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status == ESP_GATT_OK) {
			memcpy(gatt_handle_table, param->add_attr_tab.handles, sizeof(gatt_handle_table));
			esp_ble_gatts_start_service(gatt_handle_table[0]);
		}
		break;
	}
	case ESP_GATTS_WRITE_EVT: {
		if (param->write.handle == gatt_handle_table[2]) {
			if (param->write.is_prep) {
				// 长写分片累计
				g_payload_buffer.append((const char*)param->write.value, param->write.len);
				if (param->write.need_rsp) {
					esp_gatt_status_t status = ESP_GATT_OK;
					esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, nullptr);
				}
			} else {
				std::string payload;
				if (!g_payload_buffer.empty()) {
					payload.swap(g_payload_buffer);
					payload.append((const char*)param->write.value, param->write.len);
				} else {
					payload.assign((const char*)param->write.value, param->write.len);
				}
				OnReceivedCredentials(payload);
			}
		}
		break;
	}
	case ESP_GATTS_CONNECT_EVT: {
		g_conn_id = param->connect.conn_id;
		ESP_LOGI(TAG, "Client connected, conn_id=%u", g_conn_id);
		break;
	}
	case ESP_GATTS_DISCONNECT_EVT: {
		ESP_LOGI(TAG, "Client disconnected");
		g_conn_id = 0xFFFF;
		esp_ble_gap_start_advertising(&s_adv_params);
		break;
	}
	default:
		break;
	}
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
	(void)param;
	switch (event) {
	case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
		ESP_LOGI(TAG, "Adv started");
		break;
	default:
		break;
	}
}

void BleProvisioning::Start() {
	if (g_started) return;
	g_started = true;

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
	ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
	ESP_ERROR_CHECK(esp_bluedroid_init());
	ESP_ERROR_CHECK(esp_bluedroid_enable());

	esp_ble_gap_register_callback(gap_event_handler);
	esp_ble_gatts_register_callback(gatts_event_handler);
	esp_ble_gatts_app_register(0x55);

	esp_ble_adv_data_t adv_data = {};
	adv_data.set_scan_rsp = false;
	adv_data.include_txpower = true;
	adv_data.min_interval = 0x20;
	adv_data.max_interval = 0x40;
	adv_data.appearance = 0x00;
	adv_data.manufacturer_len = 0;
	adv_data.p_manufacturer_data = nullptr;
	adv_data.service_data_len = 0;
	adv_data.p_service_data = nullptr;
	adv_data.service_uuid_len = 2;
	uint8_t service_uuid[2] = { (uint8_t)(kServiceUuid & 0xFF), (uint8_t)(kServiceUuid >> 8) };
	adv_data.p_service_uuid = service_uuid;
	adv_data.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
	ESP_ERROR_CHECK(esp_ble_gap_config_adv_data(&adv_data));

	s_adv_params.adv_int_min = 0x20;
	s_adv_params.adv_int_max = 0x40;
	s_adv_params.adv_type = ADV_TYPE_IND;
	s_adv_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
	s_adv_params.channel_map = ADV_CHNL_ALL;
	s_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
	ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&s_adv_params));

	ESP_LOGI(TAG, "BLE provisioning started");
}

void BleProvisioning::Stop() {
	if (!g_started) return;
	g_started = false;
	esp_ble_gap_stop_advertising();
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();
	ESP_LOGI(TAG, "BLE provisioning stopped");
} 