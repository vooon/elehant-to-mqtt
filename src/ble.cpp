
#include "ble.h"
#include "ntp.h"
#include "mqtt_commander.h"
#include "display.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


unsigned int ble::raw_advertise_counter = 0;

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

class ElehanDataB0 {
public:

	uint8_t tariff_idx;		// SVT-15 have two measurements
	uint8_t seq;			// sequence number
	uint32_t device_num;		// meter mfg number
	uint32_t counter;		// counter in 0.1 L
	uint16_t temperature;		// temp in 0.01 C

	ElehanDataB0() :
		tariff_idx(0),
		seq(0),
		device_num(0),
		counter(0),
		temperature(0) {}

	virtual std::string message_type();
	virtual bool parse(BLEAdvertisedDevice &dev);
};

class ElehantWaterMeterAdvertismentB0 : public ElehanDataB0 {
public:
	static constexpr auto MESSAGE_TYPE = "Elehant SVD-15 B0";

	std::string message_type() override
	{
		return MESSAGE_TYPE;
	}

	bool parse(BLEAdvertisedDevice &dev) override
	{
		auto addr = dev.getAddress();
		auto esp_addr = addr.getNative();

		// NOTE: See docs/protocol.md

		// -- validate that this package can be parsed --
		if (
			memcmp(esp_addr, "\xb0\x01\x02", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x02\x02", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x03\x02", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x04\x02", 3) != 0)
			return false;

		if (!dev.haveManufacturerData())
			return false;

		auto mfg_data = dev.getManufacturerData();

		if (mfg_data.length() != 19)
			return false;

		if (memcmp(&mfg_data.front(), "\xff\xff\x80", 3) != 0)
			return false;

		// index for SVD-15: 0; for SVT-15 - 1 or 2.
		tariff_idx = 0;
		if (memcmp(esp_addr, "\xb0\x03\x02", 3) == 0)
			tariff_idx = 1;
		else if (memcmp(esp_addr, "\xb0\x04\x02", 3) == 0)
			tariff_idx = 2;

		// -- parse --
		seq = mfg_data.at(3);
		memcpy(&device_num, &mfg_data.at(8), 3);	device_num &= 0x00ffffff;
		memcpy(&counter, &mfg_data.at(11), 4);
		memcpy(&temperature, &mfg_data.at(16), 2);

		return true;
	}
};

class ElehantGasMeterAdvertismentB0 : public ElehanDataB0 {
public:
	static constexpr auto MESSAGE_TYPE = "Elehant SGBD-4 B0";

	std::string message_type() override
	{
		return MESSAGE_TYPE;
	}

	bool parse(BLEAdvertisedDevice &dev) override
	{
		auto addr = dev.getAddress();
		auto esp_addr = addr.getNative();

		// NOTE: See docs/protocol.md

		// -- validate that this package can be parsed --
		if (
			memcmp(esp_addr, "\xb0\x10\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x11\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x12\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x30\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x31\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x32\x01", 3) != 0 &&
			memcmp(esp_addr, "\xb0\x22\x01", 3) != 0)
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
		memcpy(&temperature, &mfg_data.at(16), 2);

		return true;
	}
};

class MyAdvertisedDeviceCallbacls :
	public BLEAdvertisedDeviceCallbacks
{
	void onResult(BLEAdvertisedDevice dev) override
	{
		log_d("Got advertisment");

		ble::raw_advertise_counter += 1;

		auto now = millis();
		auto ts = ntp::g_ntp.getEpochTime();
		//uint32_t now = esp_timer_get_time() / 1000;
		//auto ts = 0;

		if (cfg::ble::publish_raw) {
			send_raw(now, ts, dev);
		}

		ElehantWaterMeterAdvertismentB0 water_data;
		if (water_data.parse(dev)) {
			send_elehant_counter(now, ts, dev, water_data);
		}
		else {
			ElehantGasMeterAdvertismentB0 gas_data;
			if (gas_data.parse(dev)) {
				send_elehant_counter(now, ts, dev, gas_data);
			}
		}
	}

	void send_raw(uint32_t now, unsigned long ts, BLEAdvertisedDevice &dev)
	{
		//DynamicJsonDocument jdoc(1024);
		jdoc.clear();
		auto root = jdoc.to<JsonObject>();

		auto addr_type = dev.getAddressType();

		mqtt::json_stamp(root, now, ts);

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["addr_type"] = int(addr_type);

		// [[[cog:
		// for key, getter in [
		//     ("name", "Name", ),
		//     ("rssi", "RSSI", ),
		//     ("appearance", "Appearance", ),
		//     ("tx_power", "TXPower", ),
		//     ]:
		//     cog.outl(f"""\
		// if (dev.have{getter}())
		//         jdev["{key}"] = dev.get{getter}();
		// else
		//         jdev["{key}"] = nullptr;
		// """)
		// ]]]
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

		//[[[end]]] (checksum: 1afcd59e804d529e0471a87e9723be8d)

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

	void send_elehant_counter(uint32_t now, unsigned long ts, BLEAdvertisedDevice &dev, ElehanDataB0 &edata)
	{
		jdoc.clear();
		auto root = jdoc.to<JsonObject>();

		mqtt::json_stamp(root, now, ts);
		root["seq"] = +edata.seq;

		auto jdev = root.createNestedObject("dev");
		jdev["bdaddr"] = dev.getAddress().toString();
		jdev["device_num"] = edata.device_num;
		jdev["rssi"] = dev.getRSSI();
		jdev["message_type"] = edata.message_type();

		auto cntr = root.createNestedObject("counter");
		cntr["m3"] = edata.counter * 0.0001;
		cntr["l"] = edata.counter * 0.1;
		cntr["ml"] = uint64_t(edata.counter) * 100;

		root["temperature"] = edata.temperature * 0.01;

		mqtt::ble_report_counter(edata.device_num, edata.tariff_idx, jdoc);
		display::update_counter(now, edata.device_num, edata.tariff_idx, edata.counter, dev.getRSSI());
	}

private:
	StaticJsonDocument<2048> jdoc;
};

static void ble_thd(void *arg)
{
	log_i("BLE thread started.");

	for (;;) {
		log_i("Scanning...");

		// blocking
		pScan->start(30, true);

		//pScan->clearResults();

		delay(100);
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

	xTaskCreatePinnedToCore(ble_thd, "ble", 4096, NULL, 0, NULL, USE_CORE);
}
