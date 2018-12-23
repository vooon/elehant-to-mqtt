/**
 * OTA manager
 *
 * @author Vladimir Ermakov 2017,2018
 */

#pragma once

#include "common.h"
#include "config.h"

namespace ota {

enum Result : int {
	FAILED = 0,
	NO_UPDATES = 1,
	OK = 2,
};

String last_error();
void start_update(String url = cfg::ota::URL);

};
