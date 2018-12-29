/* That firmware is aimed to capture measurement data
 * advertised via BLE by Elehant SVD-15 water meters
 * and publish that data to MQTT.
 */

#include <WiFi.h>
#include <WiFiMulti.h>

#include "config.h"
#include "mqtt_commander.h"
#include "ntp.h"
#include "ble.h"
#include "pref_portal.h"
#include "influx.h"
#include <uptime.h>
#include <soft_wdt.h>

#include <ArduinoJson.h>

static WiFiMulti m_wifi_multi;


static void wifi_event(WiFiEvent_t event)
{
	log_d("WiFi event: %d", event);

	switch (event) {
	case SYSTEM_EVENT_STA_GOT_IP:
		{
			auto ip = WiFi.localIP();
			log_i("WIFI: Got connection, IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
			mqtt::on_wifi_state_change(true);
		}
		break;

	case SYSTEM_EVENT_STA_DISCONNECTED:
		log_e("WIFI: disconnected");
		mqtt::on_wifi_state_change(false);
		break;

	default:
		break;
	}
}

static void wifi_init()
{
	log_i("WIFI: initializing...");

	WiFi.mode(WIFI_STA);
	WiFi.onEvent(wifi_event);
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);

	auto host = cfg::get_hostname();
	WiFi.setHostname(host.c_str());

	if (cfg::pref::portal_enabled) {
		pref_portal::start_pref_portal();
	}

	if (cfg::wl::credencials.empty()) {
		pref_portal::start_wifi_ap();
		pref_portal::run();
	}

	for (auto c : cfg::wl::credencials) {
		m_wifi_multi.addAP(c.first.c_str(), c.second.c_str());
	}
}

void setup()
{
	Serial.begin(115200);
	log_i("Setup");

	cfg::init();
	uptime::init();
	ntp::init();
	wifi_init();
	mqtt::init();
	ble::init();
	influx::init();
	//soft_wdt::init();
}

void loop()
{
	m_wifi_multi.run();
	delay(1000);
}
