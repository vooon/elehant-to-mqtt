/**
 * MQTT commander
 *
 * @author Vladimir Ermakov 2017
 */

#pragma once

#include "ota.h"

namespace mqtt {

void init();
void loop();
void shedule_report_feed(bool success);
void on_wifi_state_change();
void ota_report(ota::Result result);

};
