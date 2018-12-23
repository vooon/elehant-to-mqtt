
#include "ble.h"
#include "ntp.h"
#include "mqtt_commander.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


static BLEScan *pScan;


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

class ElehantMeterAdvertisment00 {
public:
	uint8_t seq;		// sequence number
	uint32_t device_num;	// meter mfg number
	uint32_t counter;	// counter in 0.1 L

	bool parse(BLEAdvertisedDevice &dev)
	{
		auto addr = dev.getAddress();
		auto esp_addr = addr.getNative();

		// NOTE: See docs/protocol.md

		// -- validate that this package can be parsed --
		if (memcmp(esp_addr, "\xb0\x01\x02", 3) != 0)
			return false;

		if (!dev.haveManufacturerData())
			return false;

		auto mfg_data = dev.getManufacturerData();

		if (mfg_data.length() != 19)
			return false;

		if (memcmp(&mfg_data.front(), "\xff\xff\x80", 3) != 0)
			return false;

		// -- parse --
		seq = mfg_data.at(3);
		memcpy(&device_num, &mfg_data.at(8), 3);	device_num &= 0x00ffffff;
		memcpy(&counter, &mfg_data.at(11), 4);

		return true;
	}
};

class MyAdvertisedDeviceCallbacls:
	public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice dev)
	{
		log_d("Got advertisment");

		//auto now = millis();
		//auto ts = ntp::g_ntp.getEpochTime();
		uint32_t now = esp_timer_get_time() / 1000;
		auto ts = 0;

		if (1 /* TODO add flag */)
			send_raw(now, ts, dev);

		ElehantMeterAdvertisment00 elehant_data;
		if (elehant_data.parse(dev)) {
			send_elehant_counter(now, ts, dev, elehant_data);
		}
	}

	void send_raw(uint32_t now, unsigned long ts, BLEAdvertisedDevice &dev)
	{
		DynamicJsonDocument jdoc(512);
		auto root = jdoc.to<JsonObject>();

		auto addr_type = dev.getAddressType();

		root["now"] = now;
		root["ts"] = ts;

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["addr_type"] = int(addr_type);

		/*[[[cog:
		for key, getter in [
		    ("name", "Name", ),
		    ("rssi", "RSSI", ),
		    ("appearance", "Appearance", ),
		    ("tx_power", "TXPower", ),
		    ]:
		    cog.outl(f"""\
		if (dev.have{getter}())
			jdev["{key}"] = dev.get{getter}();
		else
			jdev["{key}"] = nullptr;
		""")
		]]]*/
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

		mqtt::ble_report_raw_adv(jdoc);
	}

	void send_elehant_counter(uint32_t now, unsigned long ts, BLEAdvertisedDevice &dev, ElehantMeterAdvertisment00 &edata)
	{
		DynamicJsonDocument jdoc(512);
		auto root = jdoc.to<JsonObject>();

		root["now"] = now;
		root["ts"] = ts;
		root["seq"] = +edata.seq;

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["mfg_device_num"] = edata.device_num;
		jdev["rssi"] = dev.getRSSI();
		jdev["message_type"] = "Elehant SVD-15 B0";

		auto cntr = root.createNestedObject("counter");
		cntr["m3"] = edata.counter * 0.0001;
		cntr["l"] = edata.counter * 0.1;
		cntr["ml"] = uint64_t(edata.counter) * 100;

		mqtt::ble_report_counter(edata.device_num, jdoc);
	}
};

static void ble_thd(void *arg)
{
	log_i("BLE thread started.");

	for (;;) {
		log_i("Scanning...");

		// blocking
		pScan->start(3600, true);

		//pScan->clearResults();
	}

	vTaskDelete(NULL);
}

void ble::init()
{
	BLEDevice::init("svd-15-mqtt");
	pScan = BLEDevice::getScan();
	pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacls(), true);
	pScan->setActiveScan(false);
	pScan->setInterval(100);
	pScan->setWindow(99);

	xTaskCreate(ble_thd, "ble", 4096, NULL, 3, NULL);
}
