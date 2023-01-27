SVD-15 water counter protocol decoding
======================================

*(eng. TBD)*


Расшифровка протокола водяного счетчика Элехант СВД-15
======================================================

BLE сниффером удалось определить, что счетчик шлет все данные только в пакетах advertise.
Ни каких GATT не реализовано. Пейринга - тоже.

Из дальнейшего дампа пакетов объявлений видно что используются два BD-ADDR:

- `B0:01:02:<device number>` - используется для передачи значения (частично распознано)
- `B1:01:02:<device number>` - неизвестно, но данные всегда одни и теже


В оригинальном приложении отображаются:
- Серийный номер прибора (найден)
- Показания счетчика (найден)
- Температура (для СВТ-15)
- `B0:02:02:<device number>` - аналогичен `B0:01:02` (прим. 2021, #7)


Mfg Data at B0
--------------

```
b0:01:02:00:26:e8 - "mfg_len":19, "mfg_data":"FFFF80370A010201E826003900000065D70A00"
```

Little Endian.

| bytes | type | value  | desc |
|-------|------|--------|------|
| 0-1   | u16  | 0xffff | manufacturer id \* |
| 2     | u8   | 0x80   | unknown, always 80 |
| 3     | u8   | 0x37   | some kind of sequence counter, +1 on each next pkt |
| 4-7   | u8[4] | `0A 01 02 01` | unknown, always that value |
| 8-10  | u24  | 0x0026e8 | device number, same as in BD-ADDR |
| 11-14 | u32  | 0x00000039 | counter in 0.1 L |
| 15    | u8 | 0x65 | unknown, always 65 (?) |
| 16-17 | u16 | 0x0ad7 | temperature, 0.01 C (for SVT-15) |
| 18    | u8 | 0x00 | unknown, always 00 |

\* See Supplement to the Bluetooth Core Specification, ver.7, Part.A, section 1.4: "MANUFACTURER SPECIFIC DATA".


Mfg Data at B1
--------------

```
b1:01:02:00:26:e8 - "mfg_len":19, "mfg_data":"FFFF80246C72A598DEA4CFF3D7F0DAD3BF253A"
```

Always same for one device.


Расшифровка протокола водяного счетчика Элехант СВТ-15
======================================================

Произведена автором прямой интеграции для Home Assistant [elehant_water](https://github.com/raxers/elehant_water),
[см. sensor.py](https://github.com/raxers/elehant_water/blob/4fba2e46d9344bd0e26c2c2297e4a1203e8cd119/custom_components/elehant_water/sensor.py#L39-L54).

Все отличие:

- Пакет B0 дополнен температурой (вероятно есть и у СВД-15, обновил таблицу);
- Имеет два адреса `B0:03:02` и `B0:04:02` для измерений по тарифам.


Расшифровка протокола газового счетчика Элехант СГБД-4
======================================================

Смотри тему #4.

Сообщение очень похоже на водяной счетчик. Интересующее нас число находится по тому-же смещению, и также в декалитрах.

MAC адреса приборов:

- `B0:10:01` - СГБД-1,8
- `B0:11:01` - СГБД-3,2
- `B0:12:01` - СГБД-4,0
- `B0:32:01` - СГБД-4,0ТК


Mfg Data at B0
--------------

```
B0:12:01:Xx:Yy:Zz - "mfg_len":19, "mfg_data":"FFFF80A896010112ZzYyXxF90900007F710B0F"
```

| bytes | type | value  | desc |
|-------|------|--------|------|
| 0-1   | u16  | 0xffff | manufacturer id \* |
| 2     | u8   | 0x80   | unknown, always 80 |
| 3     | u8   | 0xa8   | some kind of sequence counter, +1 on each next pkt |
| 4-7   | u8[4] | `96 01 01 12` | unknown, always that value |
| 8-10  | u24  | 0xXXYYZZ | device number, same as in BD-ADDR |
| 11-14 | u32  | 0x000009f9 | counter in 0.1 L |
| 15    | u8 | 0x7f | unknown (?) |
| 16-17 | u16 | 0x0b71 | temperature? |
| 18    | u8 | 0x0f | unknown |
