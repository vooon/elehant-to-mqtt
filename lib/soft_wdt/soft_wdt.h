
#pragma once

#include <cstdint>

[[noreturn]] void die();

namespace soft_wdt {

void init(uint32_t timeout_ms);
void feed();

}; // namespace soft_wdt
