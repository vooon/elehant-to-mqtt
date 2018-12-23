
#include "ota.h"
#include "config.h"
#include "mqtt_commander.h"

//#define DEBUG_ESP_HTTP_UPDATE		1
//#define DEBUG_ESP_PORT		Serial

using namespace ota;

#ifdef ESP8266
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>

static volatile int m_wdt_cnt;
static Ticker m_long_wdt;
#else
#include <ESP32httpUpdate.h>
#endif

static String m_ota_url = "";


Result ota::update()
{
#ifdef ESP8266
	ESP.wdtFeed();
	//ESP.wdtDisable();

	m_wdt_cnt = 120;	// Give 2min for OTA
	m_long_wdt.attach_ms(1000, []() {
		if (--m_wdt_cnt > 0)
			ESP.wdtFeed();
		});
#else
	soft_wdt::feed();	// Give 1min
#endif

	int res;
	auto current_version =
		String("fw:") + cfg::msgs::FW_VERSION +
		" hw:" + cfg::msgs::HW_VERSION +
		" id:" + cfg::mqtt::id;

#ifdef ESP8266
	current_version += " md5:" + ESP.getSketchMD5();
#endif

	Log.notice(F("Start OTA\n"));
	ESPhttpUpdate.rebootOnUpdate(false);
	if (strlen(cfg::ota::FINGERPRINT) != 0) {
		res = ESPhttpUpdate.update(m_ota_url, current_version, cfg::ota::FINGERPRINT);
	}
	else {
		res = ESPhttpUpdate.update(m_ota_url, current_version);
	}

	return static_cast<Result>(res);
}

String ota::last_error()
{
	return ESPhttpUpdate.getLastErrorString();
}

#ifdef ESP8266
void ota::start_update()
{
	mqtt::ota_report(update());
}

#else
static void updater_thd(void *arg)
{
	Log.notice(F("OTA thread started.\n"));

	auto result = update();
	Log.notice(F("OTA result: %d\n"), result);
	mqtt::ota_report(result);

	vTaskDelete(NULL);
}

void ota::start_update(String url)
{
	m_ota_url = url;
	xTaskCreate(updater_thd, "ota", 10240, NULL, 2, NULL);
}
#endif
