
#include <common.h>
#include "config.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

using namespace cfg;

constexpr auto WIFI_SSIDd = "wifi-ssid.%d";
constexpr auto WIFI_PASSWDd = "wifi-passwd.%d";
constexpr auto MQTT_CLIENT_ID = "mqtt-client-id";
constexpr auto MQTT_BROKER = "mqtt-broker";
constexpr auto MQTT_PORT = "mqtt-port";
constexpr auto MQTT_USER = "mqtt-user";
constexpr auto MQTT_PASSWD = "mqtt-passwd";
constexpr auto PREF_PORTAL_ENABLED = "pref-portal-en";

wl::vCredencials wl::credencials;
String mqtt::client_id;
String mqtt::broker;
uint16_t mqtt::port;
String mqtt::user;
String mqtt::password;
bool pref::portal_enabled = true;


static String make_client_id()
{
	char buf[128];
	uint8_t mac_arr[6];
	uint64_t mac = ESP.getEfuseMac();

	memcpy(mac_arr, &mac, sizeof(mac_arr));

	snprintf(buf, sizeof(buf) - 1,
		// [[[cog:
		// len_=6
		// fmt='%s' + '-%02X' * len_
		// args = [f'"{fmt}"', 'mqtt::ID_PREFIX'] + [f'mac_arr[{it}]' for it in range(len_)]
		// cog.outl(',\n'.join(args))
		// ]]]
		"%s-%02X-%02X-%02X-%02X-%02X-%02X",
		mqtt::ID_PREFIX,
		mac_arr[0],
		mac_arr[1],
		mac_arr[2],
		mac_arr[3],
		mac_arr[4],
		mac_arr[5]
		// [[[end]]] (checksum: 3878b7d71c9e3bfe43e0798359dddbd9)
		);

	return buf;
}

static void init_gparameters()
{
	mqtt::client_id = make_client_id();
	mqtt::broker = mqtt::D_BROKER;
	mqtt::port = mqtt::D_PORT;
	mqtt::user = mqtt::D_USER;
	mqtt::password = "";

	pref::portal_enabled = true;

	wl::credencials.clear();
}

static void update_gparameters()
{
	DynamicJsonDocument jdoc(512);

	// -- read file --
	File file = SPIFFS.open(pref::CONFIG_JSON);
	if (!file) {
		log_i("Config are empty");
		return;
	}

	auto err = deserializeJson(jdoc, file);
	file.close();

	if (err != DeserializationError::Ok) {
		log_e("JSON parse error: %s", err.c_str());
		return;
	}

	// -- check format is known --
	auto root = jdoc.as<JsonObject>();
	if (root["type"] != pref::JS_TYPE_HEADER) {
		log_e("CONFIG unknown format");
		return;
	}

	// -- use prefs --
	JsonObject prefs = root["prefs"];

	// MQTT prefs
	mqtt::client_id = prefs[MQTT_CLIENT_ID] | mqtt::client_id;
	mqtt::broker = prefs[MQTT_BROKER] | mqtt::broker;
	mqtt::port = prefs[MQTT_PORT] | mqtt::port;
	mqtt::user = prefs[MQTT_USER] | mqtt::user;
	mqtt::password = prefs[MQTT_PASSWD] | "";

	// Pref portal prefs
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
}

void cfg::init()
{
	log_i("Config init...")

	init_gparameters();

	if (!SPIFFS.begin(true)) {
		log_e("SPIFFS failed to mount!");
		die();
	}

	pinMode(io::CFG_CLEAR, INPUT_PULLUP);
	delay(1);
	if (!digitalRead(io::CFG_CLEAR)) {
		log_i("CONFIG clear forced by pin!");
		reset_and_die();
	}

	update_gparameters();

	log_i("MQTT Client ID: %s", mqtt::client_id.c_str());
}

String cfg::get_hostname()
{
	String host = cfg::mqtt::client_id;
	host.toLowerCase();
	host.replace("_", "-");
	return host;
}

void cfg::reset_and_die()
{
	log_i("Requested reset settings and die.");
	SPIFFS.remove(pref::CONFIG_JSON);
	SPIFFS.end();
	die();
}
