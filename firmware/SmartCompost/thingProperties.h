// ============================================================
//   thingProperties.h — Arduino IoT Cloud Variables
//   Auto-synced with the cloud dashboard.
//
//   HOW TO USE:
//   1. Go to cloud.arduino.cc → Things → Create Thing
//   2. Add the variables listed below with matching names/types
//   3. Copy your DEVICE_ID + SECRET_KEY into config.h
//   4. Download the generated thingProperties.h and replace this file
//      OR manually match variable names below in the Thing editor
// ============================================================

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "config.h"

// ─── Sensor Readings (Read-Only from cloud perspective) ─────
CloudTemperatureSensor temperature;       // DHT11 — °C
CloudRelativeHumidity  humidity;          // DHT11 — % RH
CloudFloat             moisture;          // Soil sensor — % moisture
CloudFloat             pm25;              // PM2.5 — µg/m³

// ─── Derived / Computed Values ──────────────────────────────
CloudFloat             healthScore;       // 0–100 composite score
CloudInt               daysElapsed;       // Days since monitoring started
CloudInt               daysRemaining;     // Predicted days until compost ready
CloudString            compostStatus;     // "Not Ready" / "Good Progress" / "Ready!"
CloudString            alerts;            // Comma-separated active alert messages

// ─── Control Variables (writeable from dashboard) ───────────
CloudBool              buzzerEnabled;     // Toggle buzzer from dashboard

// ─── Callback declarations (defined in .ino) ────────────────
void onBuzzerEnabledChange();

// ─── initProperties — called once in setup() ────────────────
void initProperties() {

  ArduinoCloud.setBoardId(DEVICE_ID);
  ArduinoCloud.setSecretDeviceKey(SECRET_KEY);

  // Register each property:
  // addProperty(variable, permission, update_policy, callback)
  ArduinoCloud.addProperty(temperature,    READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(humidity,       READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(moisture,       READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(pm25,           READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(healthScore,    READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(daysElapsed,    READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(daysRemaining,  READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(compostStatus,  READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(alerts,         READ,  ON_CHANGE, NULL);
  ArduinoCloud.addProperty(buzzerEnabled,  READWRITE, ON_CHANGE, onBuzzerEnabledChange);
}

// ─── Connection handler (WiFi) ───────────────────────────────
WiFiConnectionHandler ArduinoIoTPreferredConnection(WIFI_SSID, WIFI_PASSWORD);
