/**
 * Simple board Uptime timer
 *
 * ESP32 only - uses FreeRTOS API
 */

#pragma once

//#include "common.h"

namespace uptime {

void init();
uint64_t uptime_ms();

} // namespace uptime
