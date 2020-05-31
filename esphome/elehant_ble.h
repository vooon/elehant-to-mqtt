
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef ARDUINO_ARCH_ESP32

#define TAG "elehant"

class ElehantBLE : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
	public:
		//void set_address(uint64_t address) { address_ = address; }
		void set_serial_no(uint32_t serial_no) { serial_no_ = serial_no; }

		bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override {


			ESP_LOGD(TAG, "parse_device(): kuku! %s", device.address_str().c_str());

			// this devices have MAC starting from: B0:01:02:
			uint64_t mac = device.address_uint64();
			if ((mac & 0xffffff000000ULL) != 0xB00102000000ULL) {
				ESP_LOGVV(TAG, "parse_device(): unknown MAC address.");
				return false;
			}

			// lover part of MAC is identical to serial number
			uint32_t serial_no = mac & 0x00ffffff;
			if (serial_no != serial_no_) {
				ESP_LOGVV(TAG, "parse_device(): different serial_no: %u", serial_no)
					return false;
			}

			ESP_LOGVV(TAG, "parse_device(): MAC address %s found.", device.address_str().c_str());

			auto mfg_datas = device.get_manufacturer_datas();
			if (mfg_datas.empty()) {
				ESP_LOGD(TAG, "parse_device(): no mfg data");
			}

			return false;
		}

		float get_setup_priority() const override { return setup_priority::DATA; }

		void setup() override {}

		sensor::Sensor *total_m3_ = new sensor::Sensor();
		sensor::Sensor *total_l_ = new sensor::Sensor();
		sensor::Sensor *rssi_ = new sensor::Sensor();

	protected:
		//uint64_t address_;
		uint32_t serial_no_;
};


#endif // ARDUINO_ARCH_ESP32
