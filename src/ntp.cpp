
#include "ntp.h"
#include "config.h"
#include <common.h>

#include <WiFiUdp.h>
#include <NTPClient.h>

static WiFiUDP m_ntp_udp;
NTPClient ntp::g_ntp(m_ntp_udp, cfg::time::NTP_POOL);

static void ntp_thd(void *arg)
{
	log_i("NTP thread started.");

	// XXX FIXME
	//

	for (;;) {
		delay(1000);

		if (!WiFi.isConnected()) {
			continue;
		}

		log_i("Configuring SNTP...");

		// setup local time to UTC
		configTime(0, 0, cfg::time::NTP_POOL);

		log_i("Begin NTP...");
		ntp::g_ntp.begin();
		break;
	}

	for (;;) {
		ntp::g_ntp.update();
		delay(1000);
	}

	vTaskDelete(NULL);
}


void ntp::init()
{
	log_i("Starting NTP thread...");

	xTaskCreate(ntp_thd, "ntp", 4096, NULL, 1, NULL);
}
