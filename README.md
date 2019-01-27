
![logo: device to mqtt and influx](docs/logo.png)


Прошивка для ESP32 для сбора данных со счетчиков Элехант СВД-15
===============================================================

Так как эти счетчики выдают показания сразу через Bluetooth LE,
их довольно удобно использовать в системе умного дома.
Однако производитель не предоставляет ни какой информации об протоколе,
и единственная возможность получить показания - воспользоваться официальным приложением
или выносным дисплеем.

Так как сообщения идут в широковещательном режиме,
то без какого-либо вмешательства в ПО прибора учета,
можно получить те-же данные, что и в оригинальном приложении.

И тут очень кстати ESP32, который позволяет сделать MQTT сенсор на одном чипе.


Лицензия
--------

Лицензировано на условиях GPL v3.


Железо
------

Принципиально прошивка может работать на любой плате с ESP32, т.к. все необходимое уже встроено в чип.
Желательно иметь экран SSD1603, который есть на плате Wemos ESP32 OLED.
Но все будет работать и без него, просто без какой-либо индикации.

![photo of device running firmware](docs/IMG_20190127_181700.jpg)


TODO
----
- [x] BLE sniffer
- [x] mfg data decoder
- [x] config
- [x] WiFi
- [x] MQTT client
- [x] InfluxDB client (UDP)
- [x] Watch dog (soft wdt on WiFi connection)
- [x] NTP date
- [ ] LED indicate pkt sent (maybe)
- [x] OLED display (maybe)
- [ ] Home Assistant auto discovery (maybe)
- [x] OTA
