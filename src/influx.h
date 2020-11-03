
#pragma once

#include <common.h>
#include <ArduinoJson.h>
#include <influxdb.h>

namespace influx {

void init();
void send_status(const JsonDocument &jdoc);
void send_counter(const JsonDocument &jdoc);

};	// namespace influx
