
#include <map>

#include <SSD1306Wire.h>

#include <common.h>
#include "config.h"
#include "display.h"
#include "mqtt_commander.h"

#include "icons_xbm.h"
#include "Orbitron_Medium_8.h"
#include "Orbitron_Medium_10.h"
#include "Orbitron_Medium_16.h"
#include "Orbitron_Medium_24.h"

bool display::have_display = false;

static SSD1306Wire m_dis(0x3c, cfg::io::I2C_SDA, cfg::io::I2C_SCL);

struct CounterData {
	uint32_t last_seen;
	uint32_t device_num;
	uint32_t counter_01l;
	int rssi;
};

static SemaphoreHandle_t m_counter_mux = nullptr;
static std::map<uint32_t, CounterData> m_counter_data;

template<typename ... Args>
static String format(const char *fmt, Args ... args)
{
	char buf[128];

	::snprintf(buf, sizeof(buf) - 1, fmt, args...);
	buf[sizeof(buf) - 1] = '\0';

	return String(buf);
}

static void draw_normal()
{
	auto now = millis();

	auto wl_is_connected = WiFi.isConnected();
	auto wl_rssi = WiFi.RSSI();
	auto wl_ssid = WiFi.SSID();

	auto mq_is_connected = mqtt::is_connected();

	size_t cnt_len = 0;
	static size_t cnt_idx = 0;
	static uint32_t prev_cnt_switch = 0;
	static CounterData cnt_data {0, 0, 0, 0};

	if (xSemaphoreTake(m_counter_mux, pdMS_TO_TICKS(10)) == pdTRUE) {

		// remove outdated
		for (auto it = m_counter_data.cbegin(); it != m_counter_data.cend(); ) {
			if (now - it->second.last_seen > 60000)	// XXX
				it = m_counter_data.erase(it);
			else
				it = std::next(it);
		}

		cnt_len = m_counter_data.size();
		if (cnt_len > 0 && (now - prev_cnt_switch > 2000)) {	// XXX
			prev_cnt_switch = now;
			auto it = m_counter_data.cbegin();

			if (cnt_idx >= cnt_len)
				cnt_idx = 0;

			std::advance(it, cnt_idx++);

			cnt_data = it->second;
		}

		xSemaphoreGive(m_counter_mux);
	}

	// -*- WiFi status -*-

	m_dis.drawXbm(0, 0, 16, 16, (wl_is_connected)? icon_wifi1_bits : icon_wifi2_bits);

	uint8_t const* wl_strength = nullptr;
	if (!wl_is_connected)
		wl_strength = icon_no_con_bits;
	else if (wl_rssi > -67)
		wl_strength = icon_signal_4_bits;
	else if (wl_rssi > -70)
		wl_strength = icon_signal_3_bits;
	else if (wl_rssi > -80)
		wl_strength = icon_signal_2_bits;
	else
		wl_strength = icon_signal_1_bits;

	m_dis.drawXbm(16, 0, 16, 16, wl_strength);

	// -*- MQTT status -*-

	m_dis.drawXbm(34, 0, 16, 16, icon_talk_bits);
	m_dis.drawXbm(50, 0, 16, 16, (mq_is_connected) ? icon_check_bits : icon_no_con_bits);

	// -*- BT seen -*-
	m_dis.drawXbm(68, 0, 16, 16, icon_bluetooth_bits);

	m_dis.setTextAlignment(TEXT_ALIGN_RIGHT);
	m_dis.setFont(Orbitron_Medium_16);
	m_dis.drawString(127, 2, format("%03d", cnt_len));

	if (cnt_len == 0) {
		m_dis.setFont(Orbitron_Medium_24);
		m_dis.setTextAlignment(TEXT_ALIGN_CENTER);
		m_dis.drawString(63, 28, "NO DATA");
	}
	else {
		auto dt = now - cnt_data.last_seen;

		m_dis.setTextAlignment(TEXT_ALIGN_LEFT);
		m_dis.setFont(Orbitron_Medium_10);
		m_dis.drawString(0, 18, format("#%03d R %02d", cnt_idx, cnt_data.rssi));

		m_dis.setTextAlignment(TEXT_ALIGN_RIGHT);
		m_dis.drawString(127, 18, format("N %d", cnt_data.device_num));

		m_dis.setTextAlignment(TEXT_ALIGN_LEFT);
		m_dis.setFont(Orbitron_Medium_24);
		m_dis.drawString(0, 28, format("%05d", cnt_data.counter_01l / 10000));

		m_dis.setFont(Orbitron_Medium_10);
		m_dis.setTextAlignment(TEXT_ALIGN_RIGHT);
		m_dis.drawString(127, 28+10, format("%03d", (cnt_data.counter_01l % 10000) / 10));

		m_dis.setFont(Orbitron_Medium_10);
		m_dis.setTextAlignment(TEXT_ALIGN_LEFT);
		m_dis.drawString(0, 52, format("%2d.%1d s ago", dt / 1000, (dt % 1000) / 100));
	}
}

static void disp_thd(void *arg)
{
	log_i("Display thread started.");

	// XXX(vooon): there is no way to check presence
	display::have_display = true;

	m_dis.init();

	if (cfg::display::flip_v)	m_dis.flipScreenVertically();
	if (cfg::display::flip_h)	m_dis.mirrorScreen();

	m_dis.setFont(Orbitron_Medium_16);

	m_dis.display();
	delay(100);

	// Target FPS: 2
	constexpr uint32_t SLEEP_MS = 500;

	for (;;) {
		const uint32_t tstart = millis();
		m_dis.clear();

		draw_normal();

		m_dis.display();

		const uint32_t tdur = millis() - tstart;
		const uint32_t sleep_ms = min(SLEEP_MS, SLEEP_MS - tdur);

		log_i("draw time: %d, sleep: %d", tdur, sleep_ms);
		delay(sleep_ms);
	}

	vTaskDelete(NULL);
}

void display::init()
{
	log_i("Starting display thread...");
	m_counter_mux = xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(disp_thd, "disp", 4096, NULL, 0, NULL, USE_CORE);
}

void display::update_counter(uint32_t now, uint32_t device_num, uint32_t counter_01l, int rssi)
{
	if (xSemaphoreTake(m_counter_mux, pdMS_TO_TICKS(10)) != pdTRUE)
		return;

	m_counter_data[device_num] = CounterData{now, device_num, counter_01l, rssi};

	xSemaphoreGive(m_counter_mux);
}
