/**
 * Config module
 *
 *
 * @author Vladimir Ermakov 2018
 */

#pragma once

#include <common.h>

namespace cfg {
namespace io {
	constexpr auto CFG_CLEAR = 22;
	constexpr auto LED = 25;
#define BOARD_HAS_LED	1

	constexpr auto I2C_SDA = 4;
	constexpr auto I2C_SCL = 5;
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
	constexpr auto D_USER = "DVES_USER";

	constexpr auto ID_PREFIX = "elehant";
	constexpr auto KEEPALIVE = 30;

	extern String client_id;
	extern String broker;
	extern uint16_t port;
	extern String user;
	extern String password;
};

namespace influx {
	constexpr auto D_PORT = 8086;

	extern String addr;	//!< UDP IP addr
	extern uint16_t port;	//!< UDP listener port
};

namespace time {
	constexpr auto NTP_POOL = "ru.pool.ntp.org";
};

namespace pref {
	constexpr auto CONFIG_JSON = "/config.json";
	constexpr auto CONFIG_JSON_NEW = "/config.json.new";
	constexpr auto JS_TYPE_HEADER = "ESP-PREFERENCES-V1.0";

	extern bool portal_enabled;
};

namespace timer {
	constexpr auto STATUS_REPORT_MS = 10000;
	constexpr auto LED_BLINK_MS = 500;
};

namespace topic {
	enum class Type {
		cmnd = 0,
		stat = 1,
		tele = 3,
	};

	inline String to_string(Type t) {
		switch(t) {
			case Type::cmnd:	return "cmnd";
			case Type::stat:	return "stat";
			case Type::tele:	return "tele";
			default:		return "type-error";
		}
	}

	template<typename T>
	inline String make(Type type, T topic) {
		String topic_s(topic);
		topic_s.toUpperCase();

		return to_string(type) + "/"
			+ mqtt::client_id + "/"
			+ topic_s;
	}
};

namespace msgs {
	constexpr auto LWT_OFFLINE = "Offline";
	constexpr auto LWT_ONLINE = "Online";

	extern const char *FW_VERSION;
	constexpr auto HW_VERSION = "elehant";
};

namespace ota {
	constexpr auto URL = "http://esp.vehq.ru:8092/bin/elehant/fw.bin";
};


void init();
String get_hostname();
void reset_and_die();
String get_mac();

};
