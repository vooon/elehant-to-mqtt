
#include "common.h"
#include "config.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

using namespace cfg;

constexpr auto WIFI_SSIDd = "wifi-ssid.%d";
constexpr auto WIFI_PASSWDd = "wifi-passwd.%d";
constexpr auto MQTT_BROKER = "mqtt-broker";
constexpr auto MQTT_PORT = "mqtt-port";
constexpr auto MQTT_USER = "mqtt-user";
constexpr auto MQTT_PASSWD = "mqtt-passwd";
constexpr auto PREF_PORTAL_ENABLED = "pref-portal-en";

wl::vCredencials wl::credencials;
String mqtt::id;
String mqtt::broker;
uint16_t mqtt::port;
String mqtt::user;
String mqtt::password;
bool pref::portal_enabled = true;


static void update_gparameters()
{
	// -- set defaults --
	mqtt::broker = mqtt::D_BROKER;
	mqtt::port = mqtt::D_PORT;
	mqtt::user = mqtt::D_USER;
	mqtt::password = "";
	pref::portal_enabled = true;
	wl::credencials.clear();

	// -- read file --
	File file = SPIFFS.open(pref::CONFIG_JSON);
	if (!file) {
		Log.notice(F("Config are empty\n"));
		return;
	}

	DynamicJsonBuffer js_buffer(512);

	JsonObject &root = js_buffer.parseObject(file);
	if (!root.success()) {
		Log.error(F("JSON parse error\n"));
		return;
	}

	if (root["type"] != pref::JS_TYPE_HEADER) {
		Log.error(F("JSON unknown format\n"));
		return;
	}

	JsonObject &prefs = root["prefs"];

	mqtt::broker = prefs[MQTT_BROKER] | mqtt::D_BROKER;
	mqtt::port = prefs[MQTT_PORT] | mqtt::D_PORT;
	mqtt::user = prefs[MQTT_USER] | mqtt::D_USER;
	mqtt::password = prefs[MQTT_PASSWD] | "";
	pref::portal_enabled = prefs[PREF_PORTAL_ENABLED] | true;

	// WiFi credencials
	for (size_t i = 0; i < wl::CRED_MAX; i++) {
		String ssid, passwd;
		char key[24] = "";

		snprintf(key, sizeof(key) - 1, WIFI_SSIDd, i);
		ssid = prefs[key] | "";

		snprintf(key, sizeof(key) - 1, WIFI_PASSWDd, i);
		passwd = prefs[key] | "";

		if (ssid.length() == 0 || passwd.length() == 0)
			break;

		wl::credencials.emplace_back(ssid, passwd);
	}

	file.close();
}

void cfg::commit()
{
	// XXX do i need that func?
}

void cfg::init()
{
	using topic::make;

#ifdef ESP8266
	mqtt::id = cfg::mqtt::ID_PREFIX + String(ESP.getChipId(), 10);
#else
	uint8_t mac_arr[6];
	uint64_t mac = ESP.getEfuseMac();

	memcpy(mac_arr, &mac, sizeof(mac_arr));
	mqtt::id = cfg::mqtt::ID_PREFIX;
	for (size_t i = sizeof(mac_arr); i > 0; i--) {
		char buf[16];
		snprintf(buf, sizeof(buf) - 1, "%02X", mac_arr[i]);
		mqtt::id += buf;
	}

#endif

	Log.notice(F("MQTT ID: %s\n"), mqtt::id.c_str());

	if (!SPIFFS.begin(true)) {
		Log.fatal(F("SPIFFS failed to mount!\n"));
		die();
	}

	pinMode(io::CFG_CLEAR, INPUT_PULLUP);
	delay(1);
	if (!digitalRead(io::CFG_CLEAR)) {
		Log.notice(F("EEPROM reset forced by pin.\n"));
		reset_and_die();
	}

	update_gparameters();
}

String cfg::get_hostname()
{
	String host = cfg::mqtt::id;
	host.toLowerCase();
	host.replace("_", "-");
	return host;
}

void cfg::reset_and_die()
{
	Log.notice(F("Requested reset settings and die.\n"));
	SPIFFS.remove(pref::CONFIG_JSON);
	SPIFFS.end();
	die();
}
