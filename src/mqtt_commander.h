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
void json_stamp(JsonObject &root, uint32_t now=0, uint64_t ts=0);
void ota_report(ota::Result result);
void ble_report_raw_adv(DynamicJsonDocument jdoc);
void ble_report_counter(uint32_t device_num, DynamicJsonDocument jdoc);
bool is_connected();

};
