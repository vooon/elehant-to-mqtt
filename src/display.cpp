
#include <map>

#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <common.h>
#include "config.h"
#include "display.h"
#include "mqtt_commander.h"

#include "icons_xbm.h"

bool display::have_display = false;

static Adafruit_SSD1306 m_dis(128, 64, &Wire/*, -1, 100000UL*/);

struct CounterData {
	uint32_t last_seen;
	uint32_t device_num;
	uint32_t counter_01l;
	int rssi;
};

static SemaphoreHandle_t m_counter_mux = nullptr;
static std::map<uint32_t, CounterData> m_counter_data;


static void draw_normal()
{
	auto wl_is_connected = WiFi.isConnected();
	auto wl_rssi = WiFi.RSSI();
	auto wl_ssid = WiFi.SSID();

	auto mq_is_connected = mqtt::is_connected();

	size_t cnt_len = 0;
	static size_t cnt_idx = 0;
	CounterData cnt_data {0, 0, 0, 0};

	if (xSemaphoreTake(m_counter_mux, pdMS_TO_TICKS(10)) == pdTRUE) {
		auto now = millis();

		for (auto it = m_counter_data.cbegin(); it != m_counter_data.cend(); ) {
			if (now - it->second.last_seen > 60000)
				it = m_counter_data.erase(it);
			else
				it = std::next(it);
		}

		cnt_len = m_counter_data.size();
		if (cnt_len > 0) {
			auto it = m_counter_data.begin();

			// TODO advance to N'th element

			cnt_data = it->second;
		}

		xSemaphoreGive(m_counter_mux);
	}

	m_dis.setCursor(0, 0);
	m_dis.setTextColor(WHITE);
	m_dis.stopscroll();

	// -*- WiFi status -*-

	m_dis.drawXBitmap(0, 0, (wl_is_connected)? icon_wifi1_bits : icon_wifi2_bits, 16, 16, WHITE);

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

	m_dis.drawXBitmap(16, 0, wl_strength, 16, 16, WHITE);

	// -*- MQTT status -*-

	m_dis.drawXBitmap(34, 0, icon_talk_bits, 16, 16, WHITE);
	m_dis.drawXBitmap(50, 0, (mq_is_connected) ? icon_check_bits : icon_no_con_bits, 16, 16, WHITE);

	// -*- BT seen -*-
	m_dis.drawXBitmap(70, 0, icon_bluetooth_bits, 16, 16, WHITE);
	m_dis.setCursor(86, 2);
	m_dis.setTextSize(2);
	m_dis.printf("%03d", cnt_len);

	if (cnt_data.device_num == 0) {
		// 18x24
		m_dis.setTextSize(3);
		m_dis.setCursor(1, 28);
		m_dis.print("NO DATA");
	}
}

static void disp_thd(void *arg)
{
	log_i("Display thread started.");

	// XXX(vooon): there is no way to check presence
	display::have_display = true;

	Wire.begin(cfg::io::I2C_SDA, cfg::io::I2C_SCL);

	m_dis.begin(SSD1306_SWITCHCAPVCC, 0x3c);

	//m_dis.fillScreen(WHITE);
	m_dis.display();
	delay(1000);

	m_dis.clearDisplay();

	// Target FPS: 4
	constexpr uint32_t SLEEP_MS = 250;

	for (;;) {
		const uint32_t tstart = millis();
		m_dis.clearDisplay();

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
	xTaskCreate(disp_thd, "disp", 2048, NULL, 0, NULL);
}

void display::update_counter(uint32_t now, uint32_t device_num, uint32_t counter_01l, int rssi)
{
	if (xSemaphoreTake(m_counter_mux, pdMS_TO_TICKS(10)) != pdTRUE)
		return;

	m_counter_data[device_num] = CounterData{now, device_num, counter_01l, rssi};

	xSemaphoreGive(m_counter_mux);
}
