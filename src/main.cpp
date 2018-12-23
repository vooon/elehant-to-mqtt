/* That firmware is aimed to capture measurement data
 * advertised via BLE by Elehant SVD-15 water meters
 * and publish that data to MQTT.
 */

#include <WiFi.h>
#include <WiFiMulti.h>

#include "config.h"
#include "mqtt_commander.h"
#include "ntp.h"
#include "pref_portal.h"
#include <uptime.h>
#include <soft_wdt.h>

#include <ArduinoJson.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


static BLEScan *pScan;
static WiFiMulti m_wifi_multi;

static inline std::string to_hex(const std::string &str)
{
	std::string ret;

	ret.resize(str.length() * 2);
	auto *p = &ret.front();

	for (auto v : str) {
		p += ::sprintf(p, "%02X", v);
	}

	return ret;
}

class MyAdvertisedDeviceCallbacls:
	public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice dev)
	{
#if 0
		//StaticJsonDocument<4096> jdoc;
		DynamicJsonDocument jdoc(512);
		auto root = jdoc.to<JsonObject>();

		auto addr_type = dev.getAddressType();

		root["now"] = millis();

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["addr_type"] = int(addr_type);

		//[[[cog:
		// for key, getter in [
		//     ("name", "Name", ),
		//     ("rssi", "RSSI", ),
		//     ("appearance", "Appearance", ),
		//     ("tx_power", "TXPower", ),
		//     ]:
		//     cog.outl(f"""\
		// if (dev.have{getter}())
		// 	jdev["{key}"] = dev.get{getter}();
		// else
		// 	jdev["{key}"] = nullptr;
		// """)
		//]]]
		if (dev.haveName())
			jdev["name"] = dev.getName();
		else
			jdev["name"] = nullptr;

		if (dev.haveRSSI())
			jdev["rssi"] = dev.getRSSI();
		else
			jdev["rssi"] = nullptr;

		if (dev.haveAppearance())
			jdev["appearance"] = dev.getAppearance();
		else
			jdev["appearance"] = nullptr;

		if (dev.haveTXPower())
			jdev["tx_power"] = dev.getTXPower();
		else
			jdev["tx_power"] = nullptr;

		//[[[end]]] (checksum: 5282096a1e1a621e5ea8d29ed06de368)

		if (dev.haveManufacturerData()) {
			auto data = dev.getManufacturerData();

			jdev["mfg_len"] = data.length();
			jdev["mfg_data"] = to_hex(data);
		}

		if (dev.haveServiceUUID()) {
			jdev["srv_uuid"] = dev.getServiceUUID().toString();
		}

		serializeJson(jdoc, Serial);
		Serial.write("\n");
#endif
	}
};


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


	BLEDevice::init("svd-15-mqtt");
	pScan = BLEDevice::getScan();
	pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacls(), true);
	pScan->setActiveScan(false);
	pScan->setInterval(100);
	pScan->setWindow(99);

	cfg::init();
	uptime::init();
	wifi_init();
	mqtt::init();
	//soft_wdt::init();
}

void loop()
{
	log_i("Scanning...");
	// blocking
	pScan->start(3600, true);

	//pScan->clearResults();
}
