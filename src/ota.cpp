
#include "ota.h"
#include "config.h"
#include "mqtt_commander.h"

//#define DEBUG_ESP_HTTP_UPDATE		1
//#define DEBUG_ESP_PORT		Serial

using namespace ota;

#include <HTTPUpdate.h>

static String m_ota_url = "";
static WiFiClient m_client;


static Result update()
{
	HTTPUpdateResult res;

	auto current_version =
		String("fw:") + cfg::msgs::FW_VERSION +
		" hw:" + cfg::msgs::HW_VERSION +
		" client_id:" + cfg::mqtt::client_id;

	log_i("Starting OTA: %s", m_ota_url.c_str());

	//soft_wdt::feed();	// Give 1min

	httpUpdate.rebootOnUpdate(false);
#ifdef BOARD_HAS_LED
	httpUpdate.setLedPin(cfg::io::LED, LOW);
#endif

	res = httpUpdate.update(m_client, m_ota_url, current_version);

	return static_cast<Result>(res);
}

String ota::last_error()
{
	return httpUpdate.getLastErrorString();
}

static void updater_thd(void *arg)
{
	log_i("OTA thread started.");

	auto result = update();

	log_i("OTA result: %d", result);
	//mqtt::ota_report(result);

	vTaskDelete(NULL);
}

void ota::start_update(String url)
{
	m_ota_url = url;
	xTaskCreate(updater_thd, "ota", 10240, NULL, 2, NULL);
}
