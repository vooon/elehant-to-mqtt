/**
 * Config module
 *
 *
 * @author Vladimir Ermakov 2017,2018
 */

#pragma once

#include "common.h"

namespace cfg {
namespace io {
#if defined(BOARD_LEDMATRIX_V10)
	constexpr auto MLED_MOSI = 23;
	constexpr auto MLED_CLK = 18;
	constexpr auto MLED_CS = 5;
	constexpr auto CFG_CLEAR = 21;
#define HAS_MAX7219_SPI	1
#else
#error Unsupported board!
#endif
};

namespace wl {
	constexpr auto CONNECT_TIMEOUT_SEC = 30;
	constexpr auto SETUP_TIMEOUT_SEC = 120;
	constexpr auto CRED_MAX = 20;

	using Credencials = std::pair<String, String>;
	using vCredencials = std::vector<Credencials>;

	extern vCredencials credencials;
};

namespace mqtt {
	constexpr auto D_BROKER = "domus1";
	constexpr auto D_PORT = 1883;
	constexpr auto D_USER = "";

	constexpr auto ID_PREFIX = "ESP_";
	constexpr auto KEEPALIVE = 30;

	extern String id;
	extern String broker;
	extern uint16_t port;
	extern String user;
	extern String password;
};

namespace rtc {
	constexpr auto NTP_POOL = "ru.pool.ntp.org";
};

namespace pref {
	constexpr auto CONFIG_JSON = "/config.json";
	constexpr auto JS_TYPE_HEADER = "ESP-PREFERENCES-V1.0";

	extern bool portal_enabled;
};

namespace timer {
	constexpr auto ALIVE_PING_MS = 10000;
	constexpr auto LED_BLINK_MS = 500;
	constexpr auto SERVER_HEARTBEAT_TIMEOUT_MS = 60000;
};

namespace topic {
	constexpr auto SUB_COMMAND_PREFIX = "/command/";
	constexpr auto SUB_SERVER_ALIVE = "/server-alive";
	constexpr auto SUB_DEV_CONFIG = "/config/";
	constexpr auto SUB_TEXT_TO_SHOW = "/text-to-show/";
	constexpr auto PUB_ALIVE_PREFIX = "/alive/";
	constexpr auto PUB_ERROR_PREFIX = "/error/";
	constexpr auto PUB_SUCCESS_PREFIX = "/success/";
	constexpr auto PUB_WILL_PREFIX = "/conn-status/";
	constexpr auto PUB_STATISTICS = "/statistics/";
	constexpr auto PUB_DEV_INFO = "/dev-info/";

	template<typename T>
	inline String make(T prefix) {
		return prefix + mqtt::id;
	}
};

namespace msgs {
	constexpr auto WILL_MSG = "OFFLINE";
	constexpr auto WILL_OK_MSG = "ONLINE";

	constexpr auto SUCCESS_OK = "FOOD_NO_ERROR";
	constexpr auto SUCCESS_CMD_RECV = "COMMAND_RECEIVED";
	constexpr auto ERROR_CMD_UNK = "COMMAND_UNKNOWN";
	constexpr auto ERROR_INVAL = "EINVAL";
	constexpr auto ERROR_INVAL_JSON = "INVAL_JSON";

	constexpr auto CMD_OTA = "ota";
	constexpr auto CMD_REBOOT = "reboot";
	constexpr auto CMD_RESET_SETTINGS = "reset_settings";

	extern const char *FW_VERSION;

#if defined(BOARD_LEDMATRIX_V10)
	constexpr auto HW_VERSION = "ledmatrix-v1.0";
#endif

	constexpr auto OTA_FAILED = "OTA_FAILED";
	constexpr auto OTA_NO_UPDATES= "OTA_NO_UPDATES";
	constexpr auto OTA_STARTED = "OTA_STARTED";

	constexpr auto RST_OK = "REBOOTING";
	constexpr auto RST_CFG = "CFG_RESET";
};

namespace ota {
#if defined(BOARD_LEDMATRIX_V10)
	constexpr auto URL = "http://esp.vehq.ru:8092/bin/ledmatrix-v10/fw.bin";
#endif
};


void init();
void commit();
String get_hostname();
void reset_and_die();

};
