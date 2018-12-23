#pragma once

#ifdef ESP8266
#error ESP8266 not supported, project only aimed to ESP32
#endif

#include <array>
#include <vector>

#include <Arduino.h>
#include <WiFi.h>

extern "C" {
	#include <freertos/FreeRTOS.h>
	#include <freertos/task.h>
	#include <freertos/timers.h>
}

// common functions

[[noreturn]] void die();
