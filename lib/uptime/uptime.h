/**
 * Simple board Uptime timer
 *
 * ESP32 only - uses FreeRTOS API
 */

#pragma once

#include <cstdint>

namespace uptime {

void init();
uint64_t uptime_ms();

} // namespace uptime
