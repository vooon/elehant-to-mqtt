
#pragma once

#include <common.h>
#include <ArduinoJson.h>
#include <influxdb.h>

namespace influx {

void init();
void send_status(DynamicJsonDocument jdoc);
void send_counter(DynamicJsonDocument jdoc);

};	// namespace influx
