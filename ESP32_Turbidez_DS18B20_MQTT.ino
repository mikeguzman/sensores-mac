#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// === WiFi credentials ===
const char* ssid = "LESLIE";
const char* password = "Fifi1689";

// === HiveMQ Cloud credentials ===
const char* mqtt_server = "15676d125cca46a5a2e05cf32b313f48.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "esp32-client";
const char* mqtt_pass = "135790$Mario";

// === MQTT topics ===
const char* topic_ntu = "esp32/sensor/turbidity";
const char* topic_temp = "esp32/sensor/temperature/ds18b20";

// === Sensor configuration ===
const int TURBIDITY_PIN = 34;
const int ONE_WIRE_BUS = 4;

// MQTT
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ======================================
// WIFI
// ======================================
void setup_wifi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected");
}

// ======================================
// MQTT
// ======================================
void reconnect_mqtt() {

  while (!mqttClient.connected()) {

    Serial.print("Connecting to HiveMQ...");

    if (mqttClient.connect("esp32-client", mqtt_user, mqtt_pass)) {

      Serial.println("✅ MQTT connected");

    } else {

      Serial.print("❌ MQTT failed rc=");
      Serial.println(mqttClient.state());

      delay(5000);
    }
  }
}

// ======================================
// SETUP
// ======================================
void setup() {

  Serial.begin(115200);

  setup_wifi();

  wifiClient.setInsecure();

  mqttClient.setServer(mqtt_server, mqtt_port);

  sensors.begin();
}

// ======================================
// LOOP
// ======================================
void loop() {

  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }

  mqttClient.loop();

  // =====================
  // TURBIDEZ
  // =====================
  int raw = analogRead(TURBIDITY_PIN);

  float voltage = raw * (3.3 / 4095.0);

  float ntu = 0;

  if (voltage < 1.5) {
    ntu = 3000;
  } else {
    ntu = -1120.4 * voltage * voltage +
          5742.3 * voltage -
          4352.9;
  }

  char payload_ntu[64];

  snprintf(
    payload_ntu,
    sizeof(payload_ntu),
    "{\"voltage\":%.2f,\"ntu\":%.2f}",
    voltage,
    ntu
  );

  mqttClient.publish(topic_ntu, payload_ntu);

  Serial.printf(
    "💧 Turbidez -> Voltage: %.2f V | NTU: %.2f\n",
    voltage,
    ntu
  );

  // =====================
  // DS18B20
  // =====================
  sensors.requestTemperatures();

  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C ||
      tempC == -127.00) {

    Serial.println("⚠️ DS18B20 no detectado");

  } else {

    char payload_temp[64];

    snprintf(
      payload_temp,
      sizeof(payload_temp),
      "{\"temp_celsius\":%.2f}",
      tempC
    );

    mqttClient.publish(topic_temp, payload_temp);

    Serial.printf(
      "🌡️ DS18B20 -> %.2f °C\n",
      tempC
    );
  }

  delay(5000);
}