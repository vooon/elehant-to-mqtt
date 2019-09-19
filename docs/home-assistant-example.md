Пример настройки Home Assistant
===============================

Настройка похожа на Sonoff Tasmota, за исключением топика с данными.
Тут для удобства каждый прибор учета имеет свой топик, так что не требуется ничего настраивать на стороне есп,
она публикует все услышанные приборы.

```yaml
sensor:
- platform: mqtt
  name: Water Meter 9953
  state_topic: "tele/water-1/SNS/9953"
  unit_of_measurement: m3
  value_template: '{{ value_json.counter.m3 }}'
  availability_topic: "tele/water-1/LWT"
  payload_available: Online
  payload_not_available: Offline
```
