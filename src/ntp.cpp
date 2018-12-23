
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

	configTime(0, 0, cfg::time::NTP_POOL);	// setup local time to UTC
	ntp::g_ntp.begin();

	for (;;) {
		ntp::g_ntp.forceUpdate();
		delay(10000);
	}

	vTaskDelete(NULL);
}


void ntp::init()
{
	log_i("Enabling NTP...");

	xTaskCreate(ntp_thd, "ntp", 4096, NULL, 4, NULL);
}
