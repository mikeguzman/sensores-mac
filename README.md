# ESP32 Sensors MQTT

Integrated ESP32 sketch for reading water/environment sensors and publishing the values to HiveMQ Cloud over MQTT.

Main sketch:

- `ESP32-Sensors.ino`

Reference/test sketches:

- `ESP32_Turbidez_DS18B20_MQTT.ino`
- `PH4502C_Test/PH4502C_Test.ino`

## Sensors

The integrated sketch reads:

| Sensor | Purpose | ESP32 pin | MQTT topic |
| --- | --- | --- | --- |
| Turbidity sensor | Water turbidity / NTU | GPIO34 | `esp32/sensor/turbidity` |
| DS18B20 | Water temperature | GPIO4 | `esp32/sensor/temperature/ds18b20` |
| DHT11 | Air temperature | GPIO14 | `esp32/sensor/temperature/dht11` |
| DHT11 | Air humidity | GPIO14 | `esp32/sensor/humidity/dht11` |
| TDS sensor | Total dissolved solids / ppm | GPIO35 | `esp32/sensor/tds` |
| PH4502C | pH | GPIO32 | `esp32/sensor/ph` |

Important: GPIO35 is used by the TDS sensor, so the PH4502C sensor uses GPIO32 in the integrated sketch.

## Arduino IDE Setup

Install the ESP32 board package:

1. Open Arduino IDE.
2. Go to `Settings` / `Preferences`.
3. Add this URL to `Additional Boards Manager URLs`:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

4. Open `Tools > Board > Boards Manager`.
5. Search for `esp32`.
6. Install `esp32 by Espressif Systems`.

Select a board similar to:

- `ESP32 Dev Module`

## Required Libraries

Install these from `Tools > Manage Libraries`:

- `PubSubClient`
- `OneWire`
- `DallasTemperature`
- `DHT sensor library`
- `Adafruit Unified Sensor`

The WiFi libraries are included with the ESP32 board package.

## Wiring

Use a common ground between all sensors and the ESP32.

| Module | ESP32 connection |
| --- | --- |
| Turbidity analog output | GPIO34 |
| TDS analog output | GPIO35 |
| PH4502C PO | GPIO32 |
| DHT11 data | GPIO14 |
| DS18B20 data | GPIO4 |
| Sensor GND | ESP32 GND |
| Sensor VCC | Use the correct voltage for each module |

For the DS18B20, use a pull-up resistor between the data line and VCC. A common value is `4.7k`.

Important for PH4502C: ESP32 ADC pins must not receive more than `3.3V`. If the PH4502C analog output can exceed `3.3V`, use a voltage divider before connecting it to GPIO32.

## Upload

1. Open `ESP32-Sensors.ino` in Arduino IDE.
2. Select the ESP32 board.
3. Select the correct serial port.
4. Set Serial Monitor speed to `115200`.
5. Upload the sketch.
6. Open Serial Monitor.

Expected startup messages:

```text
Conectando a WiFi...
WiFi conectado
ESP32 sensors ready
Conectando a HiveMQ...MQTT conectado
```

Every 5 seconds the ESP32 publishes sensor readings to MQTT and prints values in Serial Monitor.

## MQTT Payloads

Example payloads:

```json
{"voltage":2.30,"ntu":123.45}
```

```json
{"temp_celsius":25.50}
```

```json
{"humidity":65.00}
```

```json
{"voltage":1.85,"ppm":840.12}
```

```json
{"voltage":2.500,"ph":7.00}
```

## Calibration Notes

The current formulas are approximate and should be calibrated with real reference values.

### pH

In `ESP32-Sensors.ino`, adjust:

```cpp
float phCalibrationOffset = 0.0;
float phNeutralVoltage = 2.5;
float phSlope = -5.70;
```

Recommended process:

1. Test the PH4502C probe with pH `7.00` buffer.
2. Let the reading stabilize.
3. Update `phNeutralVoltage` to match the measured voltage at pH `7.00`.
4. Test with pH `4.00` or pH `10.00` buffer.
5. Adjust `phSlope` if needed.

### TDS

The TDS formula is currently:

```cpp
float tds = (133.42 * voltageTds * voltageTds) -
            (255.86 * voltageTds) +
            857.39;
```

Use a known calibration solution if more accurate ppm values are needed.

### Turbidity

The turbidity formula is currently:

```cpp
float ntu = voltageTurbidity < 1.5
              ? 3000
              : -1120.4 * voltageTurbidity * voltageTurbidity +
                  5742.3 * voltageTurbidity -
                  4352.9;
```

Use known NTU samples if more accurate turbidity values are needed.

## Troubleshooting

If WiFi does not connect:

- Confirm `ssid` and `password`.
- Make sure the network is 2.4 GHz. Most ESP32 boards do not support 5 GHz WiFi.

If MQTT does not connect:

- Confirm the HiveMQ host, port, username, and password.
- The sketch uses port `8883` with `wifiClient.setInsecure()` for TLS testing without a certificate.

If analog values look wrong:

- Confirm the sensor output does not exceed `3.3V`.
- Confirm each analog sensor is on an ADC1 pin.
- Keep WiFi away from ADC2 pins; this sketch uses ADC1 pins for analog sensors.

If DS18B20 is not detected:

- Confirm the `4.7k` pull-up resistor.
- Confirm data is connected to GPIO4.
- Confirm the sensor wiring order, because DS18B20 modules and bare probes may differ.
