
#include "common.h"
#include "mqtt_commander.h"
#include "config.h"
#include "ota.h"
#include <uptime.h>

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
	m_mqtt_client.publish(cfg::topic::make(type, topic).c_str(), qos, retain, value.c_str(), value.length());
}

static void pub_topic(TT type, String topic, const DynamicJsonDocument jdoc, MQTT::Qos qos=MQTT::QOS0, bool retain=false)
{
	String value;
	serializeJson(jdoc, value);

	m_mqtt_client.publish(cfg::topic::make(type, topic).c_str(), qos, retain, value.c_str(), value.length());
}

static void pub_device_info()
{
	DynamicJsonDocument jdoc(128);
	JsonObject root = jdoc.as<JsonObject>();

	auto fw = root.createNestedObject("fw");
	fw["version"] = cfg::msgs::FW_VERSION;
	fw["md5"] = ESP.getSketchMD5();
	fw["sdk-version"] = ESP.getSdkVersion();

	auto hw = root.createNestedObject("hw");
	hw["board"] = cfg::msgs::HW_VERSION;
	hw["cycle-count"] = ESP.getCycleCount();
	hw["chip-rev"] = +ESP.getChipRevision();

	pub_topic(TT::stat, "INFO", jdoc);
}

static void pub_stats()
{
	DynamicJsonDocument jdoc(128);
	JsonObject root = jdoc.as<JsonObject>();

	auto t_f = temprature_sens_read();
	float t_c = (t_f - 32.0f) / 1.8f;

	root["now"] = millis();
	root["uptime"] = (long unsigned int) uptime::uptime_ms() / 1000;
	root["wifi-rssi"] = WiFi.RSSI();
	root["wifi-ssid"] = WiFi.SSID();
	root["hall-sensor"] = hallRead();
	root["cpu-temp"] = t_c;

	//String epoch(g_ntp.getEpochTime(), 10);

	pub_topic(TT::stat, "DEVICE", jdoc);
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
	JsonObject root = jdoc.as<JsonObject>();

	log_i("SUB: %s (qos: %d, dup: %d, retain: %d, sz: %d:%d:%d) => %s",
			c_topic, properties.qos, properties.dup, properties.retain,
			len, index, total,
			payload.c_str());

	auto slash_pos = topic.rfind('/');
	auto command = topic.substr(slash_pos);
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

	m_mqtt_client.subscribe(cfg::topic::make(TT::cmnd, "#").c_str(), MQTT::QOS2);

	xTimerStart(m_report_stats_tmr, 0);
	tmr_report_stats(nullptr);
}

static void on_mqtt_disconnect(AsyncMqttClientDisconnectReason reason)
{
	log_e("MQTT connection lost. Reason: %d", reason);

	xTimerStop(m_report_stats_tmr, 0);
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
	switch (result) {
	case ota::OK:
		die();
		break;

	case ota::NO_UPDATES:
		//pub_success(cfg::msgs::OTA_NO_UPDATES);
		break;

	case ota::FAILED:
		//pub_error(cfg::msgs::OTA_FAILED, ota::last_error());
		break;
	}
}
