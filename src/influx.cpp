
#include "influx.h"
#include "config.h"
#include <AsyncUDP.h>

static AsyncUDP m_influx_udp;
static IPAddress m_influx_addr;
static bool is_influx_enabled = false;
static String m_hostname;
static String m_mac;


static inline void send(InfluxDBBuffer &buf)
{
	const auto str = buf.string();
	m_influx_udp.writeTo(reinterpret_cast<const uint8_t *>(str.c_str()), str.length(), m_influx_addr, cfg::influx::port);

	log_i("INFLUX: %s", str.c_str());
}

void influx::send_status(DynamicJsonDocument jdoc)
{
	InfluxDBBuffer buf;
	auto root = jdoc.as<JsonObject>();

	if (!is_influx_enabled && WiFi.isConnected())
		return;

	buf.begin(TIMESTAMP_USE_SERVER_TIME, "esp_status")
		.tag(TAG_HOST, m_hostname)
		.tag("mac", m_mac)
		.tag("dev_type", "water_meter_proxy")
		.value("uptime", root["uptime"].as<int>())
		.value("wifi_rssi", root["wifi_rssi"].as<int>())
		.value("hall_sensor", root["hall_sensor"].as<int>())
		.value("cpu_temp", root["cpu_temp"].as<float>())
		.end();

	send(buf);
}

void influx::send_counter(DynamicJsonDocument jdoc)
{
	InfluxDBBuffer buf;
	auto root = jdoc.as<JsonObject>();

	if (!is_influx_enabled && WiFi.isConnected())
		return;

	buf.begin(TIMESTAMP_USE_SERVER_TIME, "water_meter")
		.tag(TAG_HOST, m_hostname)
		.tag("mac", m_mac)
		.tag("bdaddr", root["dev"]["bdaddr"].as<String>())
		.tag("device_num", root["dev"]["device_num"].as<int>())
		.value("seq", root["seq"].as<int>())
		.value("rssi", root["dev"]["rssi"].as<int>())
		.value("total_m3", root["counter"]["m3"].as<float>())
		.end();

	send(buf);
}

void influx::init()
{
	log_i("Initializing InfluxDB...");

	is_influx_enabled = m_influx_addr.fromString(cfg::influx::addr);
	m_hostname = cfg::get_hostname();
	m_mac = cfg::get_mac();

	if (is_influx_enabled) {
		log_i("InfluxDB addr: %s:%u", m_influx_addr.toString().c_str(), cfg::influx::port);
	}
	else {
		log_i("InfluxDB disabled.");
	}
}
