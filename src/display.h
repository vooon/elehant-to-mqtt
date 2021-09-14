
#pragma once

namespace display {

extern bool have_display;

void init();

void update_counter(uint32_t now, uint32_t device_num, uint8_t tariff_idx, uint32_t counter_01l, int rssi);

};	// namespace display
