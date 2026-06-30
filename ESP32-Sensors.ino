#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"

// === WiFi credentials ===
const char* ssid = "LESLIE";
const char* password = "Fifi1689";

// === HiveMQ Cloud credentials ===
const char* mqtt_server = "15676d125cca46a5a2e05cf32b313f48.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "esp32-client";
const char* mqtt_pass = "135790$Mario";

// === MQTT topics ===
const char* topic_turbidity = "esp32/sensor/turbidity";
const char* topic_temp_ds18b20 = "esp32/sensor/temperature/ds18b20";
const char* topic_temp_dht11 = "esp32/sensor/temperature/dht11";
const char* topic_humidity_dht11 = "esp32/sensor/humidity/dht11";
const char* topic_tds = "esp32/sensor/tds";
const char* topic_ph = "esp32/sensor/ph";

// === Sensor pins ===
const int TURBIDITY_PIN = 34;
const int TDS_PIN = 35;
const int PH_PIN = 32;
const int DHT_PIN = 14;
const int ONE_WIRE_BUS = 4;

// === ADC settings ===
const float ADC_MAX = 4095.0;
const float ADC_REFERENCE_V = 3.3;

// === PH4502C calibration ===
const int PH_SAMPLES = 20;
const int PH_SAMPLE_DELAY_MS = 10;

// Adjust these after testing with pH 7 and pH 4 or pH 10 buffer.
float phCalibrationOffset = 0.0;
float phNeutralVoltage = 2.5;
float phSlope = -5.70;

// === Sensors ===
DHT dht(DHT_PIN, DHT11);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// === MQTT ===
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

void setup_wifi() {
  Serial.print("Conectando a WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

void reconnect_mqtt() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a HiveMQ...");

    if (mqttClient.connect("esp32-client", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT conectado");
    } else {
      Serial.print("Error MQTT, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

int readAverageAdc(int pin, int samples, int sampleDelayMs) {
  long sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(sampleDelayMs);
  }

  return sum / samples;
}

float adcToVoltage(int adcValue) {
  return adcValue * (ADC_REFERENCE_V / ADC_MAX);
}

float voltageToPh(float voltage) {
  return 7.0 + ((voltage - phNeutralVoltage) * phSlope) + phCalibrationOffset;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(TURBIDITY_PIN, ADC_11db);
  analogSetPinAttenuation(TDS_PIN, ADC_11db);
  analogSetPinAttenuation(PH_PIN, ADC_11db);

  dht.begin();
  ds18b20.begin();

  setup_wifi();

  wifiClient.setInsecure();  // For testing without a TLS certificate.
  mqttClient.setServer(mqtt_server, mqtt_port);

  Serial.println();
  Serial.println("ESP32 sensors ready");
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }

  mqttClient.loop();

  // === Turbidity ===
  int rawTurbidity = analogRead(TURBIDITY_PIN);
  float voltageTurbidity = adcToVoltage(rawTurbidity);
  float ntu = voltageTurbidity < 1.5
                ? 3000
                : -1120.4 * voltageTurbidity * voltageTurbidity +
                    5742.3 * voltageTurbidity -
                    4352.9;

  char payloadTurbidity[64];
  snprintf(
    payloadTurbidity,
    sizeof(payloadTurbidity),
    "{\"voltage\":%.2f,\"ntu\":%.2f}",
    voltageTurbidity,
    ntu
  );
  mqttClient.publish(topic_turbidity, payloadTurbidity);
  Serial.printf("Turbidez: %.2f V | %.2f NTU\n", voltageTurbidity, ntu);

  // === DS18B20 ===
  ds18b20.requestTemperatures();
  float tempDs18b20 = ds18b20.getTempCByIndex(0);

  if (tempDs18b20 == DEVICE_DISCONNECTED_C || tempDs18b20 == -127.00) {
    Serial.println("DS18B20 no detectado");
  } else {
    char payloadDs18b20[64];
    snprintf(
      payloadDs18b20,
      sizeof(payloadDs18b20),
      "{\"temp_celsius\":%.2f}",
      tempDs18b20
    );
    mqttClient.publish(topic_temp_ds18b20, payloadDs18b20);
    Serial.printf("Temp DS18B20: %.2f C\n", tempDs18b20);
  }

  // === DHT11 ===
  float humidity = dht.readHumidity();
  float tempDht11 = dht.readTemperature();

  if (!isnan(humidity) && !isnan(tempDht11)) {
    char payloadHumidity[64];
    snprintf(
      payloadHumidity,
      sizeof(payloadHumidity),
      "{\"humidity\":%.2f}",
      humidity
    );
    mqttClient.publish(topic_humidity_dht11, payloadHumidity);

    char payloadTempDht11[64];
    snprintf(
      payloadTempDht11,
      sizeof(payloadTempDht11),
      "{\"temp_celsius\":%.2f}",
      tempDht11
    );
    mqttClient.publish(topic_temp_dht11, payloadTempDht11);

    Serial.printf("Humedad DHT11: %.2f %% | Temp DHT11: %.2f C\n", humidity, tempDht11);
  } else {
    Serial.println("Error al leer el DHT11");
  }

  // === TDS ===
  int rawTds = analogRead(TDS_PIN);
  float voltageTds = adcToVoltage(rawTds);
  float tds = (133.42 * voltageTds * voltageTds) -
              (255.86 * voltageTds) +
              857.39;

  char payloadTds[64];
  snprintf(
    payloadTds,
    sizeof(payloadTds),
    "{\"voltage\":%.2f,\"ppm\":%.2f}",
    voltageTds,
    tds
  );
  mqttClient.publish(topic_tds, payloadTds);
  Serial.printf("TDS: %.2f V | %.2f ppm\n", voltageTds, tds);

  // === PH4502C ===
  int rawPh = readAverageAdc(PH_PIN, PH_SAMPLES, PH_SAMPLE_DELAY_MS);
  float voltagePh = adcToVoltage(rawPh);
  float ph = voltageToPh(voltagePh);

  char payloadPh[64];
  snprintf(
    payloadPh,
    sizeof(payloadPh),
    "{\"voltage\":%.3f,\"ph\":%.2f}",
    voltagePh,
    ph
  );
  mqttClient.publish(topic_ph, payloadPh);
  Serial.printf("pH: ADC %d | %.3f V | %.2f\n", rawPh, voltagePh, ph);

  delay(5000);
}
