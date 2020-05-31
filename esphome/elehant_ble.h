
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef ARDUINO_ARCH_ESP32

class ElehantBLE : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
public:
	//void set_address(uint64_t address) { address_ = address; }
	void set_serial_no(uint32_t serial_no) { serial_no_ = serial_no; }

	bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;

	float get_setup_priority() const override { return setup_priority::DATA; }

	sensor::Sensor *total_m3_=new sensor::Sensor();
	sensor::Sensor *total_l_=new sensor::Sensor();

protected:
	//uint64_t address_;
	uint32_t serial_no_;
};


#endif // ARDUINO_ARCH_ESP32
