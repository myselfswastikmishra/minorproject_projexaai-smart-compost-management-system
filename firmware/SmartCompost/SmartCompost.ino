// ============================================================
//   SmartCompost.ino — Main Sketch
//   IoT-Based Smart Composting System
//
//   Author  : Swastik Mishra
//   Roll    : 2401010019
//   Branch  : B.Tech CSE (2nd Year)
//   Mentor  : Dr. Manish Kumar
//   Uni     : K.R. Mangalam University, Gurgaon
//   Year    : 2024–28
//
//   Hardware:
//     • ESP32 (38-pin / 30-pin dev board)
//     • DHT11 — Temperature + Humidity (GPIO4)
//     • GP2Y1010AU0F PM2.5 — Air Quality  (GPIO34 analog, GPIO2 LED)
//     • Capacitive Soil Moisture Sensor   (GPIO35 analog)
//     • Buzzer / Status LED               (GPIO5 / GPIO18)
//
//   Dependencies (install via Library Manager):
//     • ArduinoIoTCloud      (1.15.0+)
//     • Arduino_ConnectionHandler (0.8.0+)
//     • DHT sensor library   (Adafruit, 1.4.4+)
//     • Adafruit Unified Sensor (1.1.9+)
//
//   Arduino IoT Cloud Setup:
//     1. cloud.arduino.cc → Create Thing
//     2. Add all variables listed in thingProperties.h
//     3. Copy Device ID + Secret Key into config.h
//     4. Build dashboard widgets using same variable names
// ============================================================

// ─── Arduino IoT Cloud & WiFi ───────────────────────────────
#include "thingProperties.h"

// ─── Sensor & Prediction modules ────────────────────────────
#include "sensors.h"
#include "prediction.h"
#include "config.h"

// ─── EEPROM for persisting start-time across reboots ────────
#include <EEPROM.h>
#define EEPROM_SIZE         8
#define EEPROM_ADDR_START   0   // Stores 4-byte Unix-like day counter

// ─── Runtime state ──────────────────────────────────────────
static SensorData      sensorData;
static PredictionResult prediction;

static unsigned long lastSensorReadMs   = 0;
static unsigned long lastCloudSyncMs    = 0;
static unsigned long lastPredictionMs   = 0;
static unsigned long monitoringStartMs  = 0;

static bool firstCloudSync = true;
static bool compostReadyAlerted = false;

// ─── Forward declarations ────────────────────────────────────
void loadOrInitStartTime();
void saveStartTime(unsigned long ms);
void updateStatusLED(float healthScore, bool isReady);
void soundReadyAlert();
void onBuzzerEnabledChange();  // IoT Cloud callback

// ============================================================
//   SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1500);  // Let serial monitor connect

  Serial.println();
  Serial.println(F("╔══════════════════════════════════════╗"));
  Serial.println(F("║   SmartCompost IoT — Starting...     ║"));
  Serial.println(F("║   ESP32 + DHT11 + PM2.5 + Soil       ║"));
  Serial.println(F("╚══════════════════════════════════════╝"));

  // ── EEPROM ────────────────────────────────────────────────
  EEPROM.begin(EEPROM_SIZE);
  loadOrInitStartTime();

  // ── Sensors ───────────────────────────────────────────────
  sensorsInit();

  // ── Prediction engine ─────────────────────────────────────
  predictionInit(monitoringStartMs);

  // ── Arduino IoT Cloud ─────────────────────────────────────
  initProperties();          // Defined in thingProperties.h

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Optional: set debug output level
  // 0 = none, 1 = errors, 2 = warnings, 3 = info, 4 = debug
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Initial values so dashboard isn't blank at first connect
  temperature    = 0.0f;
  humidity       = 0.0f;
  moisture       = 0.0f;
  pm25           = 0.0f;
  healthScore    = 0.0f;
  daysElapsed    = 0;
  daysRemaining  = MAX_COMPOST_DAYS;
  compostStatus  = "Initializing...";
  alerts         = "Starting up";
  buzzerEnabled  = true;

  Serial.println(F("[SETUP] Setup complete. Entering main loop."));
}

// ============================================================
//   LOOP
// ============================================================
void loop() {
  // Keep Arduino IoT Cloud connection alive
  ArduinoCloud.update();

  unsigned long nowMs = millis();

  // ── 1. Read Sensors (every SENSOR_READ_INTERVAL_MS) ────────
  if (nowMs - lastSensorReadMs >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadMs = nowMs;

    bool ok = sensorsRead(sensorData);
    if (ok) {
      sensorsPrint(sensorData);
    } else {
      Serial.println(F("[LOOP] All sensor reads failed — check connections!"));
    }
  }

  // ── 2. Run Prediction (every PREDICTION_INTERVAL_MS) ───────
  if (nowMs - lastPredictionMs >= PREDICTION_INTERVAL_MS) {
    lastPredictionMs = nowMs;

    int days = predictionGetDaysElapsed();
    predictionRun(sensorData, days, prediction);
    predictionPrint(prediction);

    // Trigger buzzer alert if compost is newly ready
    if (prediction.stage == 4 && !compostReadyAlerted && buzzerEnabled) {
      soundReadyAlert();
      compostReadyAlerted = true;
    }
    // Reset alert if we go back below ready (edge case: restarted monitoring)
    if (prediction.stage < 4) {
      compostReadyAlerted = false;
    }

    // Update status LED
    updateStatusLED(prediction.healthScore, prediction.stage == 4);
  }

  // ── 3. Sync to Arduino IoT Cloud ───────────────────────────
  if (firstCloudSync || (nowMs - lastCloudSyncMs >= CLOUD_SYNC_INTERVAL_MS)) {
    lastCloudSyncMs = nowMs;
    firstCloudSync  = false;

    // Push sensor readings
    temperature   = sensorData.dhtOk  ? sensorData.temperatureC : temperature;
    humidity      = sensorData.dhtOk  ? sensorData.humidityPct  : humidity;
    moisture      = sensorData.soilOk ? sensorData.moisturePct  : moisture;
    pm25          = sensorData.pm25Ok ? sensorData.pm25_ugm3    : pm25;

    // Push prediction outputs
    healthScore   = prediction.healthScore;
    daysElapsed   = predictionGetDaysElapsed();
    daysRemaining = prediction.daysRemaining;
    compostStatus = prediction.status;
    alerts        = prediction.alerts;

    Serial.print  (F("[CLOUD] Synced → Status: "));
    Serial.print  (prediction.status);
    Serial.print  (F(" | Health: "));
    Serial.print  (prediction.healthScore, 0);
    Serial.print  (F(" | Days Rem: "));
    Serial.println(prediction.daysRemaining);
  }
}

// ============================================================
//   loadOrInitStartTime
//   Reads saved start time from EEPROM. If empty (0xFFFFFFFF),
//   sets monitoring start to now and saves it.
//   This ensures daysElapsed survives power cycles.
// ============================================================
void loadOrInitStartTime() {
  uint32_t saved = 0;
  EEPROM.get(EEPROM_ADDR_START, saved);

  if (saved == 0xFFFFFFFF || saved == 0) {
    // First boot — start monitoring now
    monitoringStartMs = 0;  // Will be set relative to current millis
    predictionInit(millis());
    uint32_t nowSec = (uint32_t)(millis() / 1000UL);
    EEPROM.put(EEPROM_ADDR_START, nowSec);
    EEPROM.commit();
    Serial.println(F("[EEPROM] New monitoring session started. Timer reset."));
  } else {
    // Restore relative start
    // saved = seconds into monitoring when last saved (approximate)
    monitoringStartMs = millis() - ((unsigned long)saved * 1000UL);
    Serial.print(F("[EEPROM] Resumed monitoring. Days elapsed ≈ "));
    Serial.println(saved / 86400UL);
  }
}

// ============================================================
//   updateStatusLED
//   Green = ready, Yellow = good, Red = needs attention
//   Uses single-color LED on STATUS_LED_PIN (blink pattern)
// ============================================================
void updateStatusLED(float score, bool isReady) {
  if (isReady) {
    // Rapid blink = ready!
    for (int i = 0; i < 3; i++) {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(100);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(100);
    }
  } else if (score >= SCORE_GOOD_THRESHOLD) {
    // Slow single blink = good progress
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(200);
    digitalWrite(STATUS_LED_PIN, LOW);
  } else {
    // Double blink = needs attention
    for (int i = 0; i < 2; i++) {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(150);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(150);
    }
  }
}

// ============================================================
//   soundReadyAlert — plays a ready melody on buzzer
// ============================================================
void soundReadyAlert() {
  if (!buzzerEnabled) return;
  Serial.println(F("[ALERT] *** COMPOST IS READY! *** Sounding buzzer."));

  // Simple beep pattern: 3 long beeps
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
}

// ============================================================
//   onBuzzerEnabledChange — Arduino IoT Cloud callback
//   Called automatically when the cloud variable changes
// ============================================================
void onBuzzerEnabledChange() {
  Serial.print(F("[CLOUD] Buzzer toggled: "));
  Serial.println(buzzerEnabled ? F("ON") : F("OFF"));
  if (!buzzerEnabled) {
    digitalWrite(BUZZER_PIN, LOW);  // Make sure it's off
  }
}
