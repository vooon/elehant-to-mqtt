/* That firmware is aimed to capture measurement data
 * advertised via BLE by Elehant SVD-15 water meters
 * and publish that data to MQTT.
 */

#include <WiFi.h>

#include <ArduinoJson.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <base64.h>

BLEScan *pScan;

template<typename T>
static inline std::string to_hex(T &str)
{
	std::string ret;

	ret.resize(str.length() * 2);
	auto *p = &ret.front();

	for (char v : str) {
		*p++ = ((v >> 4) & 0xff) + '0';
		*p++ = (v        & 0xff) + '0';
	}

	return ret;
}

class MyAdvertisedDeviceCallbacls:
	public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice dev)
	{
		StaticJsonDocument<4096> jdoc;
		auto root = jdoc.to<JsonObject>();

		auto addr_type = dev.getAddressType();

		root["now"] = millis();

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["addr_type"] = addr_type;

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
	}
};

void setup()
{
	Serial.begin(115200);
	log_i("Setup");

	BLEDevice::init("");
	pScan = BLEDevice::getScan();
	pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacls(), true);
	pScan->setActiveScan(false);
	pScan->setInterval(100);
	pScan->setWindow(99);
}

void loop()
{
	log_i("Scanning...");
	BLEScanResults devices = pScan->start(10, true);

	//log_d("Scan result:");
	devices.dump();

	//pScan->clearResults();
}
