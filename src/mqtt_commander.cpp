
#include "common.h"
#include "mqtt_commander.h"
#include "config.h"
//#include "feed_motor.h"
#include "ota.h"
//#include "i2c_scan.h"
//#include "sound.h"
#include <uptime.h>
//#include "ledmatrix.h"

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

template<typename T>
static void pub_topic(T prefix, String value, MQTT::Qos qos=MQTT::QOS0, bool retain=false)
{
	//m_mqtt_client.publish(cfg::topic::make(prefix).c_str(), qos, retain, value.c_str(), value.length());
}

static void pub_success(String msg, String arg1 = String())
{
	if (arg1.length() != 0) {
		msg += " " + arg1;
	}

	//Log.notice(F("Pub success: %s\n"), msg.c_str());
	//pub_topic(cfg::topic::PUB_SUCCESS_PREFIX, msg, MQTT::QOS2);
}

static void pub_error(String msg, String desc = String())
{
	if (desc.length() != 0) {
		msg += " " + desc;
	}

	//Log.error(F("Pub error: %s\n"), msg.c_str());
	//pub_topic(cfg::topic::PUB_ERROR_PREFIX, msg, MQTT::QOS2);
}

static void pub_device_info()
{
#if 0
	StaticJsonBuffer<256> jbuf;

	auto &root = jbuf.createObject();

	auto &fw = root.createNestedObject("fw");
	fw["version"] = cfg::msgs::FW_VERSION;
	//fw["md5"] = ESP.getSketchMD5(); // XXX FIXME
	fw["sdk-version"] = ESP.getSdkVersion();

	auto &hw = root.createNestedObject("hw");
	hw["board"] = cfg::msgs::HW_VERSION;
	hw["actuator-type"] = feedmotor::get_actuator_variant();
	hw["cycle-count"] = ESP.getCycleCount();
	hw["chip-rev"] = +ESP.getChipRevision();

	if (sound::module_present) {
		hw["snd-type"] = sound::get_module_type();
	}

	if (!i2cscan::last_scan_results.empty()) {
		auto &i2cslaves = hw.createNestedArray("i2c-slaves");

		for (auto addr : i2cscan::last_scan_results)
			i2cslaves.add(addr);
	}

	String devinfo; root.printTo(devinfo);

	pub_topic(cfg::topic::PUB_DEV_INFO, devinfo, MQTT::QOS1);

	Log.trace(F("Device Info: %s\n"), devinfo.c_str());
#endif
}

static void pub_stats()
{
#if 0
	StaticJsonBuffer<256> jbuf;

	auto &root = jbuf.createObject();

	root["now"] = millis();
	root["uptime"] = (long unsigned int) uptime::uptime_ms() / 1000;
	root["feed-counter"] = feedmotor::feed_counter;
	root["wifi-rssi"] = WiFi.RSSI();
	root["wifi-ssid"] = WiFi.SSID();
#ifdef ESP8266
	root["board-voltage"] = ESP.getVcc();
#else
	root["hall-sensor"] = hallRead();

	auto t_f = temprature_sens_read();
	float t_c = (t_f - 32.0f) / 1.8f;
	root["cpu-temp"] = t_c;
#endif

	String epoch(g_ntp.getEpochTime(), 10);
	String stats; root.printTo(stats);

	pub_topic(cfg::topic::PUB_ALIVE_PREFIX, epoch);
	pub_topic(cfg::topic::PUB_STATISTICS, stats);

	Log.trace(F("Update alive: %s\n"), epoch.c_str());
	Log.trace(F("Stats: %s\n"), stats.c_str());
#endif
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
	std::vector<char> payload(len + 1);
	memcpy(payload.data(), c_payload, len);
	payload[len] = '\0';	// force line termination

#if 0
	//Log.trace(F("SUB: %s (qos: %d, dup: %d, retain: %d, sz: %d:%d:%d) => %s\n"),
	//		c_topic, properties.qos, properties.dup, properties.retain,
	//		len, index, total,
	//		payload.data());

	if (0 == strcmp(c_topic, cfg::topic::SUB_SERVER_ALIVE)) {
		soft_wdt::feed();
		return;
	}
	else if (0 == strcmp(c_topic, cfg::topic::make(cfg::topic::SUB_DEV_CONFIG).c_str())) {
		dev_configure(payload);
		return;
	}
	else if (0 != strncmp(c_topic, cfg::topic::SUB_COMMAND_PREFIX, strlen(cfg::topic::SUB_COMMAND_PREFIX))) {
		Log.error(F("Unexpected topic subs.\n"));
		return;
	}

	std::vector<String> argv;
	argv.reserve(2);

	char *p = payload.data();
	char *saveptr = nullptr;
	for (;;) {
		auto tok = strtok_r(p, " \t", &saveptr);
		p = nullptr;

		if (tok == nullptr) {
			break;
		}

		argv.emplace_back(tok);
	}

	if (argv.empty()) {
		pub_error(cfg::msgs::ERROR_INVAL, "incorrect payload");
		return;
	}

	if (argv[0] == cfg::msgs::CMD_OTA) {
		pub_success(cfg::msgs::OTA_STARTED);
		if (argv.size() > 1)
			ota::start_update(argv[1]);
		else
			ota::start_update();
	}
	else if (argv[0] == cfg::msgs::CMD_REBOOT) {
		pub_success(cfg::msgs::RST_OK);
		die();
	}
	else if (argv[0] == cfg::msgs::CMD_RESET_SETTINGS) {
		pub_success(cfg::msgs::RST_CFG);
		cfg::reset_and_die();
	}
	else if (argv[0] == cfg::msgs::CMD_STEPS) {
		if (argv.size() < 2) {
			pub_error(cfg::msgs::ERROR_INVAL, "args");
			return;
		}

		auto steps = strtol(argv[1].c_str(), nullptr, 0);
		pub_success(cfg::msgs::STEPS_Q, String(steps));
		feedmotor::rotate_steps(steps, true, false);
	}
	else {
		pub_error(cfg::msgs::ERROR_CMD_UNK);
	}
#endif
}

static void on_mqtt_connect(bool session_present)
{
	log_i("MQTT connected. Session present: %d", session_present);

	// Publish ONLINE, board info
	//pub_topic(cfg::topic::PUB_WILL_PREFIX, cfg::msgs::WILL_OK_MSG, MQTT::QOS1, true);
	pub_device_info();

	//m_mqtt_client.subscribe(cfg::topic::make(cfg::topic::SUB_COMMAND_PREFIX).c_str(), MQTT::QOS2);
	//m_mqtt_client.subscribe(cfg::topic::SUB_SERVER_ALIVE, MQTT::QOS0);
	//m_mqtt_client.subscribe(cfg::topic::make(cfg::topic::SUB_DEV_CONFIG).c_str(), MQTT::QOS2);

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

	auto will_topic = cfg::topic::make(TT::stat, "LWT");

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

void mqtt::on_wifi_state_change()
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
