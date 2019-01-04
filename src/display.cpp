
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <common.h>
#include "config.h"
#include "display.h"

bool display::have_display = false;

static Adafruit_SSD1306 m_dis(128, 64, &Wire);

static void draw_normal()
{
	auto wl_is_connected = WiFi.isConnected();
	auto wl_rssi = WiFi.RSSI();
	auto wl_ssid = WiFi.SSID();

	m_dis.setCursor(0, 0);
	m_dis.print("C: ");
	m_dis.print(wl_is_connected);

	m_dis.print(" R: ");
	m_dis.print(wl_rssi);
}

static void disp_thd(void *arg)
{
	log_i("Display thread started.");

	// XXX(vooon): there is no way to check presence
	display::have_display = true;

	Wire.begin(cfg::io::I2C_SDA, cfg::io::I2C_SCL);
	Wire.setClock(100000);
	//Wire.setClock(400000);

	m_dis.begin(SSD1306_SWITCHCAPVCC, 0x3c);
	m_dis.fillScreen(WHITE);
	m_dis.display();
	m_dis.fillScreen(BLACK);

	for (;;) {
		auto tstart = millis();

		draw_normal();
		m_dis.display();

		auto tend = millis();

		log_i("draw time: %d", tend - tstart);
		delay(1000);
	}

	vTaskDelete(NULL);
}




void display::init()
{
	xTaskCreate(disp_thd, "disp", 2048, NULL, 0, NULL);
}
