
#include "pref_portal.h"
#include "config.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <DNSServer.h>
#include <SPIFFS.h>

constexpr auto P_HTTP = 80;
constexpr auto P_DNS = 53;

static constexpr auto INDEX_HTML =
R"(
<!DOCTYPE html>
<html>
<header>
	<title>Upload config.json</title>
</header>
<body>
	<h1>Upload config.json</h1>
	<form method="post" action="/config.json" enctype="application/x-www-form-urlencoded">
		<input type="file" name="file">
		<input type="submit" value="Submit">
	</form>
	<form method="post" action="/cfg_reset">
		<input type="submit" value="Reset prefs">
	</form>
	<form method="post" action="/reboot">
		<input type="submit" value="Reboot">
	</form>
</body>
</html>
)";


static AsyncWebServer m_http(P_HTTP);
static DNSServer m_dns;


/**
 * Is string looks like IP?
 */
static bool is_ip(const String s)
{
	for (size_t i = 0; i < s.length(); i++) {
		auto c = s.charAt(i);
		if (c != '.' && (c < '0' || c > '9'))
			return false;
	}

	return true;
}

static bool captive_portal(AsyncWebServerRequest *req)
{
	if (is_ip(req->host())) {
		return false;
	}

	auto ip = WiFi.softAPIP();
	char url[64] = "";

	snprintf(url, sizeof(url) - 1, "http://%d.%d.%d.%d/", ip[0], ip[1], ip[2], ip[3]);

	Log.notice(F("PREF: request redirect to captive portal\n"));
	req->redirect(url);

	return true;
}

static void handle_root(AsyncWebServerRequest *req)
{
	if (captive_portal(req))
		return;

	Log.trace(F("PREF: handle_root()\n"));

	req->send(200, "text/html", INDEX_HTML);
}

static void handle_reboot(AsyncWebServerRequest *req)
{
	Log.trace(F("PREF: handle_reboot()\n"));

	// XXX make some time for response
	die();

	req->send(200, "text/plain", "OK");
}

static void handle_cfg_reset(AsyncWebServerRequest *req)
{
	Log.trace(F("PREF: handle_cfg_reset()\n"));

	// XXX make some time for response
	cfg::reset_and_die();

	req->send(200, "text/plain", "RESET PREFS OK");
}

static void handle_config_json_get(AsyncWebServerRequest *req)
{
	Log.trace(F("PREF: handle_config_json_get()\n"));

	req->send(SPIFFS, cfg::pref::CONFIG_JSON, "application/json");
}

static void handle_config_json_post(AsyncWebServerRequest *req)
{
	Log.trace(F("PREF: handle_config_json_post()\n"));

	String error_msg;
	auto body_p = req->getParam(0);
	if (body_p == nullptr) {
		Log.trace("body_p null\n");
		return;
	}

	auto body = body_p->value();

	Log.trace(F("POST Body: %s\n---\n"), body_p->name().c_str());
	Serial.println(body);
	Serial.println("---");

	DynamicJsonBuffer js_buffer(512);
	JsonObject &js_root = js_buffer.parseObject(body);

	if (!js_root.success()) {
		error_msg = "JSON parse failed";
		goto error_out;
	}

	if (js_root["type"] != cfg::pref::JS_TYPE_HEADER) {
		error_msg = "No type header";
		goto error_out;
	}

	{
		File file = SPIFFS.open(cfg::pref::CONFIG_JSON, "w");
		if (!file) {
			error_msg = "file open error";
		}
		else {
			js_root.printTo(file);
			file.close();
		}
	}

error_out:
	if (error_msg.length() != 0) {
		Log.error(F("PREF: upload error: %s\n"), error_msg.c_str());
		req->send(400, "text/palin", error_msg);
	}
	else {
		req->send(200, "text/plain", "OK");
	}
}

static void handle_404(AsyncWebServerRequest *req)
{
	String text = "File not found\nURL: ";
	text += req->url();
	text += "\nMETHOD: ";
	text += (req->method() == HTTP_POST) ? "POST" : "GET";

	auto resp = req->beginResponse(404, "text/plain", text);
	resp->addHeader("Cache-Control", "no-cache");
	req->send(resp);
}

void pref_portal::start_wifi_ap()
{
	Log.notice(F("PREF: starting AP mode\n"));

	auto host = cfg::get_hostname();

	WiFi.mode(WIFI_AP);
	WiFi.softAP(host.c_str());
	WiFi.softAPsetHostname(host.c_str());

	auto ap_ip = WiFi.softAPIP();

	Log.notice(F("AP IP: "));
	Serial.println(ap_ip);

	// Setup DNS to redirect all domains to captive portal
	m_dns.setErrorReplyCode(DNSReplyCode::NoError);
	m_dns.start(P_DNS, "*", ap_ip);
}

void pref_portal::start_pref_portal()
{
	Log.notice(F("PREF: Starting portal\n"));

	m_http.on("/", HTTP_GET, handle_root);
	m_http.on("/reboot", HTTP_POST, handle_reboot);
	m_http.on("/cfg_reset", HTTP_POST, handle_cfg_reset);
	m_http.on("/config.json", HTTP_GET, handle_config_json_get);
	m_http.on("/config.json", HTTP_POST, handle_config_json_post);

	// Captive portal detector
	m_http.on("/fwlink", HTTP_GET, handle_root);		// Microsoft
	//m_http.on("/generate_204", HTTP_GET, handle_root);	// Android/Chrome OS

	m_http.onNotFound(handle_404);

	m_http.begin();
	Log.notice(F("PREF: HTTP server started\n"));
}

void pref_portal::run()
{
	Log.notice(F("PREF: lock in AP mode.\n"));
	for (;;) {
		m_dns.processNextRequest();
		delay(1);
	}
}
