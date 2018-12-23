
#include "ntp.h"
#include "config.h"

#include <WiFiUdp.h>
#include <NTPClient.h>

static WiFiUDP m_ntp_udp;
NTPClient g_ntp(m_ntp_udp, cfg::time::NTP_POOL);


void ntp::init()
{
	//Log.notice(F("Enabling NTP...\n"));
	//g_ntp.begin();
}
