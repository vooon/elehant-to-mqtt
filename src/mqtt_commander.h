/**
 * MQTT commander
 *
 * @author Vladimir Ermakov 2017
 */

#pragma once

#include <ArduinoJson.h>
#include "ota.h"

namespace mqtt {

void init();
void loop();
void shedule_report_feed(bool success);
void on_wifi_state_change(bool connected);
void ota_report(ota::Result result);
void ble_report_raw_adv(DynamicJsonDocument jdoc);
void ble_report_counter(uint32_t device_num, DynamicJsonDocument jdoc);

};
