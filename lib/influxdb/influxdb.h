/**
 * InfluxDB helper library.
 *
 * @author Vladimir Ermakov <vooon341@gmail.com>
 * @copyright 2017 Vladimir Ermakov <vooon341@gmail.com>
 */

#pragma once

constexpr auto INFLUXDB_SAFE_MTU = 1240;

// common tags
constexpr auto TAG_HOST = "host";
constexpr auto TAG_INSTANCE = "instance";
constexpr auto TAG_TYPE = "type";
constexpr auto TAG_TYPE_INSTANCE = "type_instance";

// Use InfluxDB server time for measurement.
constexpr uint64_t TIMESTAMP_USE_SERVER_TIME = 0;

/**
 * Help to construct InfluxDB line protocol format
 *
 * Member functions should be called only in this sequence:
 *
 * - begin
 * - tag...
 * - value...
 * - end
 */
class InfluxDBBuffer {
public:
	InfluxDBBuffer(size_t reserve=INFLUXDB_SAFE_MTU) :
		str(), ts_ns(0), stage(Stage::end)
	{
		str.reserve(reserve);
	}

	String string()
	{
		return str;
	}

	size_t length()
	{
		return str.length();
	}

	void clear()
	{
		str = "";
		stage = Stage::end;
	}

	InfluxDBBuffer& begin(uint64_t time_stamp_ns, String measurement)
	{
		//assert(stage == Stage::end);

		ts_ns = time_stamp_ns;
		str += measurement;
		stage = Stage::begin;

		return *this;
	}

	template<typename valueT>
	InfluxDBBuffer& tag(String tag, valueT value)
	{
		//assert(stage == Stage::begin || stage == Stage::tag);

		str += "," + tag + "=" + to_str(value);
		stage = Stage::tag;

		return *this;
	}

	template<typename valueT>
	InfluxDBBuffer& value(String name, valueT value)
	{
		//assert(stage == Stage::tag || stage == Stage::value);

		if (stage == Stage::tag)
			str += " ";
		else
			str += ",";

		str += name + "=" + to_str(value);
		stage = Stage::value;

		return *this;
	}

	InfluxDBBuffer& end()
	{
		//assert(stage == Stage::value);

		char buf[64] = "\n";

		if (ts_ns != TIMESTAMP_USE_SERVER_TIME) {
			// XXX ERROR: arduino do not support 64-bit integer to string conversions!
			snprintf(buf, sizeof(buf), " %llu\n", ts_ns);
		}

		str += buf;
		stage = Stage::end;

		return *this;
	}

private:
	String str;
	uint64_t ts_ns;

	enum class Stage {
		begin,
		tag,
		value,
		end
	};

	Stage stage;

	String to_str(const int v)
	{
		return String(v) + "i";
	}

	String to_str(const unsigned int v)
	{
		return String(v) + "i";
	}

	String to_str(const bool v)
	{
		return (v) ? "true" : "false";
	}

	String to_str(const float v)
	{
		return String(v, 6);
	}

	String to_str(String v)
	{
		v.replace(" ", "\\ ");
		v.replace(",", "\\,");
		v.replace("=", "\\=");
		v.replace("\"", "\\\"");

		return v;
	}

	String to_str(const char *v)
	{
		return to_str(String(v));
	}
};
