
#include "common.h"
#include "mqtt_commander.h"
#include "config.h"
#include "ota.h"
#include "ntp.h"
#include "ble.h"
#include "influx.h"
#include <uptime.h>

#include <rom/rtc.h>

#include <AsyncMqttClient.h>
#include <ArduinoJson.h>

#ifndef ESP8266
extern "C" uint8_t temprature_sens_read();
#endif

static AsyncMqttClient m_mqtt_client;

static TimerHandle_t m_mqtt_reconnect_tmr;
static TimerHandle_t m_report_stats_tmr;

using TT = cfg::topic::Type;

namespace MQTT {
enum Qos : uint8_t {
	QOS0 = 0,
	QOS1,
	QOS2
};
} // namespace MQTT

static void pub_topic(TT type, String topic, const String &value, MQTT::Qos qos=MQTT::QOS0, bool retain=false)
{
	auto full_topic = cfg::topic::make(type, topic);

	log_d("PUB (s): %s (q:%d,r:%d) = %s", full_topic.c_str(), qos, retain, value.c_str());
	m_mqtt_client.publish(full_topic.c_str(), qos, retain, value.c_str(), value.length());
}

static void pub_topic(TT type, String topic, const DynamicJsonDocument jdoc, MQTT::Qos qos=MQTT::QOS0, bool retain=false)
{
	String value;

	auto full_topic = cfg::topic::make(type, topic);
	serializeJson(jdoc, value);

	log_d("PUB (j): %s (q:%d,r:%d) = %s", full_topic.c_str(), qos, retain, value.c_str());
	m_mqtt_client.publish(full_topic.c_str(), qos, retain, value.c_str(), value.length());
}

void mqtt::json_stamp(JsonObject &root, uint32_t now, uint64_t ts)
{
	if (0 == now) {
		now = millis();
		//now = esp_timer_get_time() / 1000;
	}

	if (0 == ts)
		ts = ntp::g_ntp.getEpochTime();

	root["now"] = now;
	root["ts"] = ts;
}

static void pub_device_info()
{
	DynamicJsonDocument jdoc(512);
	JsonObject root = jdoc.to<JsonObject>();

	mqtt::json_stamp(root);

	auto fw = root.createNestedObject("fw");
	fw["version"] = cfg::msgs::FW_VERSION;
	fw["md5"] = ESP.getSketchMD5();	// XXX only present in fresh platform, with HTTPUpdate
	fw["sdk_version"] = ESP.getSdkVersion();

	auto hw = root.createNestedObject("hw");
	hw["board"] = cfg::msgs::HW_VERSION;
	hw["boot_count"] = cfg::get_boot_count();
	hw["mac"] = cfg::get_mac();
	hw["chip_rev"] = +ESP.getChipRevision();
	hw["core0_rst"] = +rtc_get_reset_reason(0);
	hw["core1_rst"] = +rtc_get_reset_reason(1);

	pub_topic(TT::stat, "INFO", jdoc);
}

static void pub_stats()
{
	DynamicJsonDocument jdoc(256);
	JsonObject root = jdoc.to<JsonObject>();

	auto t_f = temprature_sens_read();
	float t_c = (t_f - 32.0f) / 1.8f;

	mqtt::json_stamp(root);
	root["uptime"] = uptime::uptime_ms() / 1000.0;
	root["wifi_rssi"] = WiFi.RSSI();
	root["wifi_ssid"] = WiFi.SSID();
	root["hall_sensor"] = hallRead();
	root["cpu_temp"] = t_c;
	root["ble_adv_cnt"] = ble::raw_advertise_counter;
	root["ram_free"] = esp_get_free_heap_size();

	pub_topic(TT::stat, "DEVICE", jdoc);
	influx::send_status(jdoc);
}

static void tmr_report_stats(TimerHandle_t timer)
{
	pub_stats();
}

static void tmr_mqtt_reconnect(TimerHandle_t timer)
{
	log_i("MQTT: Connecting to: %s:%d", cfg::mqtt::broker.c_str(), cfg::mqtt::port);
	m_mqtt_client.connect();
}

static void on_mqtt_message(char *c_topic, char *c_payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
	std::string topic(c_topic);
	std::string payload(c_payload, len);
	DynamicJsonDocument jdoc(128);
	JsonObject root = jdoc.to<JsonObject>();

	log_i("SUB: %s (qos: %d, dup: %d, retain: %d, sz: %d:%d:%d) => %s",
			c_topic, properties.qos, properties.dup, properties.retain,
			len, index, total,
			payload.c_str());

	auto slash_pos = topic.rfind('/');
	auto command = topic.substr(slash_pos + 1);
	for (auto &c : command) {
		c = ::toupper(c);
	}

	if (command == "OTAURL") {
		root[command] = "Started";

		if (payload.length()) {
			ota::start_update(payload.c_str());
		}
		else {
			ota::start_update();
		}
	}
	else if (command == "INFO") {
		root[command] = "OK";
		pub_device_info();
		pub_stats();
	}
	else if (command == "CFG_CLEAR") {
		cfg::reset_and_die();	// noreturn
	}
	else if (command == "REBOOT") {
		die();			// noreturn
	}
	else {
		root["COMMAND_UNKNOWN"] = command;
	}

	pub_topic(TT::stat, "RESULT", jdoc);
}

static void on_mqtt_connect(bool session_present)
{
	log_i("MQTT connected. Session present: %d", session_present);

	// Publish ONLINE
	pub_topic(TT::tele, "LWT", String(cfg::msgs::LWT_ONLINE), MQTT::QOS1, true);

	pub_device_info();

	auto all_commands_topic = cfg::topic::make(TT::cmnd, "#");

	log_i("Subscribing to: %s", all_commands_topic.c_str());
	m_mqtt_client.subscribe(all_commands_topic.c_str(), MQTT::QOS2);

	xTimerStart(m_report_stats_tmr, 0);
	tmr_report_stats(nullptr);
}

static void on_mqtt_disconnect(AsyncMqttClientDisconnectReason reason)
{
	log_e("MQTT connection lost. Reason: %d", reason);

	xTimerStop(m_report_stats_tmr, 0);
	xTimerStart(m_mqtt_reconnect_tmr, 0);
}

void mqtt::init()
{
	log_i("Initializing MQTT...");

	auto will_topic = cfg::topic::make(TT::tele, "LWT");

	m_mqtt_client
		.onConnect(on_mqtt_connect)
		.onDisconnect(on_mqtt_disconnect)
		.onMessage(on_mqtt_message)
		.setClientId(cfg::mqtt::client_id.c_str())
		.setKeepAlive(cfg::mqtt::KEEPALIVE)
		.setCleanSession(true)			// default - true
		// strdup() required because class stores pointer, not copy of the string
		.setWill(strdup(will_topic.c_str()), MQTT::QOS1, true, cfg::msgs::LWT_OFFLINE)
		.setServer(cfg::mqtt::broker.c_str(), cfg::mqtt::port)
		;

	if (cfg::mqtt::user.length() > 0)
		m_mqtt_client.setCredentials(cfg::mqtt::user.c_str(), cfg::mqtt::password.c_str());

	m_mqtt_reconnect_tmr = xTimerCreate("mqtt-reconnect", pdMS_TO_TICKS(2000),
			pdFALSE, NULL, tmr_mqtt_reconnect);

	m_report_stats_tmr = xTimerCreate("report-stats", pdMS_TO_TICKS(cfg::timer::STATUS_REPORT_MS),
			pdTRUE, NULL, tmr_report_stats);
}

void mqtt::on_wifi_state_change(bool connected_)
{
	auto connected = WiFi.isConnected();

	if (connected) {
		log_i("MQTT: run reconnect.");
		xTimerStart(m_mqtt_reconnect_tmr, 0);
	}
	else {
		log_i("MQTT: stop reconnect.");
		xTimerStop(m_mqtt_reconnect_tmr, 0);
	}
}

void mqtt::ota_report(ota::Result result)
{
	bool reboot = false;
	DynamicJsonDocument jdoc(128);
	JsonObject root = jdoc.to<JsonObject>();

	mqtt::json_stamp(root);

	switch (result) {
	case ota::OK:
		root["OTA"] = "OK";
		reboot = true;
		break;

	case ota::NO_UPDATES:
		root["OTA"] = "NO UPDATES";
		break;

	case ota::FAILED:
		root["OTA"] = "FAILED";
		root["error"] = ota::last_error();
		break;
	}

	pub_topic(TT::stat, "RESULT", jdoc);

	if (reboot) {
		delay(20);	// XXX give some time to send RESULT
		die();
	}
}

void mqtt::ble_report_raw_adv(DynamicJsonDocument jdoc)
{
	pub_topic(TT::tele, "BLE_ADV_RAW", jdoc, MQTT::QOS0);
}

void mqtt::ble_report_counter(uint32_t device_num, DynamicJsonDocument jdoc)
{
	pub_topic(TT::tele, "SNS/" + String(device_num, 10), jdoc, MQTT::QOS1, true);
	influx::send_counter(jdoc);
}

bool mqtt::is_connected()
{
	return m_mqtt_client.connected();
}
