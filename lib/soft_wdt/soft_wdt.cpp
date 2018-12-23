
#include "soft_wdt.h"
#include <Arduino.h>

extern "C" {
	#include <freertos/FreeRTOS.h>
	#include <freertos/timers.h>
}


static TimerHandle_t m_timeout_tmr;


static void tmr_timeout(TimerHandle_t timer)
{
	log_e("SoftWDT: timed out!");
	die();
}


void soft_wdt::feed()
{
	xTimerReset(m_timeout_tmr, 0);

	//ESP.wdtFeed();
}

void soft_wdt::init(uint32_t timeout_ms)
{
	m_timeout_tmr = xTimerCreate("soft-wdt", pdMS_TO_TICKS(timeout_ms),
			pdFALSE, NULL, tmr_timeout);
	xTimerStart(m_timeout_tmr, 0);

//#ifdef ESP8266
//	ESP.wdtEnable(WDTO_2S);
//#endif
}
