
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


			ESP_LOGI(TAG, "parse_device(): kuku! %s", device.address_str().c_str());

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
				ESP_LOGE(TAG, "parse_device(): no mfg data");
			}
			if (mfg_datas.size() > 1) {
				ESP_LOGW(TAG, "mfg adv datas len: %d", mfg_datas.size());
			}

			auto &mfg_data = mfg_datas[0];

			ESP_LOGI(TAG, "d uuid= %d", mfg_data.uuid.to_string().c_str());
			ESP_LOGI(TAG, "d len= %d", mfg_data.data.size());

			if (!mfg_data.uuid.contains(0xff, 0xff)) {
				ESP_LOGE(TAG, "parse_device(): unexpected manufacturer data uuid: %s", mfg_data.uuid.to_string().c_str());
				return false;
			}
			if (mfg_data.data.size() != 17) {
				ESP_LOGE(TAG, "parse_device(): manufacturer data length %d != 17 bytes", mfg_data.data.size());
				return false;
			}
			if (mfg_data.data[0] != 0x80) {
				ESP_LOGE(TAG, "parse_device(): manufacturer data: wrong start byte");
				return false;
			}

			uint8_t seq;
			uint32_t device_num;
			uint32_t counter;

			seq = mfg_data.at(1);
			memcpy(&device_num, &mfg_data.data.at(2), 3); device_num &= 0x00ffffff;
			memcpy(&counter, &mfg_data.data.at(5), 4);

			if (serial_no != device_num) {
				ESP_LOGE(TAG, "parse_device(): serial number from MAC and data do not match, likely that format is changed. %#06 != %#06", serial_no, device_num);
				return false;
			}

			ESP_LOGI(TAG, "parse_device(): seq = %u", seq);

			total_m3_->publish_state(counter / 0.0001);
			total_l_->publish_state(counter / 0.1);
			rssi_->publish_state(device.get_rssi());

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
