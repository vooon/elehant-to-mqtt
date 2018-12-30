Configuring ESP
===============

Configuration stored in SPIFFS in JSON file.
Without that file esp will start in config mode - it will start Access Point
and serve very simple preferences portal which allow you to upload new `config.json`.

Configuration may be deleted (cleared) by:

- Ground `cfg::io::CFG_CLEAR` (GPIO22) pin and power on
- In pref-portal by pressing `Reset prefs` button
- Publish '1' to `cmnd/<client-id>/CFG_CLEAR`


Preferences portal
------------------

Intended to be Captive Portal, but for some reason don't work...
So you have manually go to `http://192.168.4.1/` after you connect to AP.

As alternative you can GET/POST JSON to `http://192.168.4.1/config.json`.
Note that portal by default stay enabled, so you can GET/POST new config in STA mode.


Options
-------

### WiFi credentials

- `wifi-ssid.%d` - SSID %d
- `wifi-passwd.%d` - Password %d

You can specify up to `cfg::wl::MAX_CRED = 20` networks.
Device will then try to connect to AP with best RSSI.


### MQTT

- `mqtt-broker` - IP or Hostname
- `mqtt-port` - port, default 1883
- `mqtt-client-id` - that string will be used as: hostname, mqtt client id, part of topic names
- `mqtt-user` - username [opt]
- `mqtt-passwd` - password [opt]

NOTE: firmware currently do not support SSL,
and it is unlikely that be changed.


### InfluxDB

- `influx-addr` - IP address of InfluxDB [opt]
- `influx-port` - UDP Port used by InfluxDB

Note: you should specially setup UDP listener in InfluxDB config.

If address not given - metrics sender will be disabled.


### Pref portal

- `pref-portal-enabled` - allow you to disable pref portal
