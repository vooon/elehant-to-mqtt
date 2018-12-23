
#include "uptime.h"

static volatile uint64_t m_uptime = 0;
static uint32_t m_last_millis = 0;

static TimerHandle_t m_uptime_tmr;


static void tmr_uptime(TimerHandle_t timer)
{
	uint32_t now = millis();

	m_uptime += (now - m_last_millis);
	m_last_millis = now;
}

void uptime::init()
{
	m_uptime_tmr = xTimerCreate("uptime", pdMS_TO_TICKS(1000), pdTRUE, NULL, tmr_uptime);
	xTimerStart(m_uptime_tmr, 0);
}

uint64_t uptime::uptime_ms()
{
	return m_uptime;
}
