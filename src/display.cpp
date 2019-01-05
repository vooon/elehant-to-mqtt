
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <common.h>
#include "config.h"
#include "display.h"

#include "icons_xbm.h"

bool display::have_display = false;

static Adafruit_SSD1306 m_dis(128, 64, &Wire/*, -1, 100000UL*/);


static void draw_normal()
{
	auto wl_is_connected = WiFi.isConnected();
	auto wl_rssi = WiFi.RSSI();
	auto wl_ssid = WiFi.SSID();

	m_dis.setCursor(0, 0);
	m_dis.setTextColor(WHITE);
	m_dis.stopscroll();

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

//m_dis.print(" AP: ");
//	m_dis.print(wl_ssid);
}

static void disp_thd(void *arg)
{
	log_i("Display thread started.");

	// XXX(vooon): there is no way to check presence
	display::have_display = true;

	Wire.begin(cfg::io::I2C_SDA, cfg::io::I2C_SCL);

	m_dis.begin(SSD1306_SWITCHCAPVCC, 0x3c);
	m_dis.fillScreen(WHITE);
	m_dis.display();
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
	xTaskCreate(disp_thd, "disp", 2048, NULL, 0, NULL);
}
