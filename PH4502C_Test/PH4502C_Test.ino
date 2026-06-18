#include <Arduino.h>

// ==========================================================
// PH4502C pH Sensor Test Sketch for ESP32
// ==========================================================
// Wiring:
//   PH4502C V+  -> 5V or 3V3, depending on your module
//   PH4502C GND -> ESP32 GND
//   PH4502C PO  -> ESP32 GPIO35 by default
//
// Use an ADC1 pin on ESP32: GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, or GPIO39.
// Avoid ADC2 pins if you later add WiFi.

const int PH_PIN = 35;

const int ADC_RESOLUTION = 4095;
const float ADC_REFERENCE_VOLTAGE = 3.3;

// Change these after calibration.
// First test with pH 7.00 buffer and adjust PH7_VOLTAGE to the printed average.
// Then test with pH 4.00 or pH 10.00 buffer and adjust PH_SLOPE if needed.
const float PH7_VOLTAGE = 2.50;
const float PH_SLOPE = -5.70;

const int SAMPLES = 20;
const unsigned long READ_INTERVAL_MS = 1000;

float readAverageVoltage() {
  unsigned long rawSum = 0;

  for (int i = 0; i < SAMPLES; i++) {
    rawSum += analogRead(PH_PIN);
    delay(10);
  }

  float rawAverage = rawSum / (float)SAMPLES;
  return rawAverage * (ADC_REFERENCE_VOLTAGE / ADC_RESOLUTION);
}

float voltageToPh(float voltage) {
  return 7.0 + ((voltage - PH7_VOLTAGE) * PH_SLOPE);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(PH_PIN, ADC_11db);

  Serial.println();
  Serial.println("PH4502C pH Sensor Test");
  Serial.println("----------------------");
  Serial.println("Open Serial Monitor at 115200 baud.");
  Serial.println("Let the probe stabilize before trusting the pH value.");
  Serial.println();
}

void loop() {
  float voltage = readAverageVoltage();
  float ph = voltageToPh(voltage);

  Serial.print("ADC pin: GPIO");
  Serial.print(PH_PIN);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | pH: ");
  Serial.println(ph, 2);

  delay(READ_INTERVAL_MS);
}
