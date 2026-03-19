// ============================================================
//   sensors.cpp — Sensor Implementations
//   DHT11 | PM2.5 GP2Y1010AU0F | Capacitive Soil Moisture
// ============================================================

#include "sensors.h"

// ─── DHT11 instance ─────────────────────────────────────────
static DHT dht(DHT_PIN, DHT_TYPE);

// ─── Moving-average buffers for PM2.5 (reduce noise) ────────
#define PM25_BUF_SIZE  8
static float   pm25Buffer[PM25_BUF_SIZE];
static uint8_t pm25BufIdx  = 0;
static bool    pm25BufFull = false;

// ─── Moving-average buffers for Soil ────────────────────────
#define SOIL_BUF_SIZE  5
static float   soilBuffer[SOIL_BUF_SIZE];
static uint8_t soilBufIdx  = 0;
static bool    soilBufFull = false;

// ────────────────────────────────────────────────────────────
//   sensorsInit
// ────────────────────────────────────────────────────────────
void sensorsInit() {
  // DHT11
  dht.begin();
  Serial.println(F("[SENSOR] DHT11 initialised on GPIO " xstr(DHT_PIN)));

  // PM2.5 — LED control pin
  pinMode(PM25_LED_PIN, OUTPUT);
  digitalWrite(PM25_LED_PIN, HIGH);  // LED off initially (active-LOW for GP2Y)

  // Soil moisture — ADC pin (input-only on ESP32)
  pinMode(SOIL_AOUT_PIN, INPUT);
  pinMode(PM25_AOUT_PIN, INPUT);

  // Buzzer & Status LED
  pinMode(BUZZER_PIN,    OUTPUT);
  pinMode(STATUS_LED_PIN,OUTPUT);
  digitalWrite(BUZZER_PIN,    LOW);
  digitalWrite(STATUS_LED_PIN, LOW);

  // Initialise moving-average buffers to zero
  memset(pm25Buffer, 0, sizeof(pm25Buffer));
  memset(soilBuffer, 0, sizeof(soilBuffer));

  Serial.println(F("[SENSOR] All sensors initialised."));
  delay(2000);  // DHT11 stabilisation time
}

// ────────────────────────────────────────────────────────────
//   pm25RawToUgm3
//   Sharp GP2Y1010AU0F conversion formula
//   Reference: Sharp Application Note AN-PSD-003
//
//   Vout  = ADC_raw * (Vcc / ADC_MAX)
//   Dust  = (Vout - 0.0) / 0.005  [V per µg/m³ sensitivity = 0.5mV/(µg/m³)]
//   Offset voltage (Voc) ≈ 0.6V at zero dust — subtract for calibration
// ────────────────────────────────────────────────────────────
float pm25RawToUgm3(int rawADC) {
  if (rawADC < 0) return 0.0f;

  // Convert raw ADC to voltage
  float voltage = rawADC * (PM25_VCC / (float)PM25_ADC_MAX);

  // GP2Y1010AU0F: Voc ≈ 0.6V (no-dust baseline), sensitivity = 0.5mV/(µg/m³)
  // dustDensity = (Vout - Voc) / sensitivity
  const float Voc         = 0.60f;  // zero-dust output voltage (calibrate if needed)
  const float sensitivity = 0.0005f; // V per µg/m³  (0.5 mV/µg/m³)

  float dustDensity = (voltage - Voc) / sensitivity;

  // Clamp to valid range
  if (dustDensity < 0.0f)   dustDensity = 0.0f;
  if (dustDensity > 500.0f) dustDensity = 500.0f;

  return dustDensity;
}

// ────────────────────────────────────────────────────────────
//   soilRawToPercent
//   Maps raw ADC reading to moisture percentage.
//   SOIL_DRY_RAW (air) → 0%,  SOIL_WET_RAW (water) → 100%
//   Calibrate SOIL_DRY_RAW and SOIL_WET_RAW in config.h
// ────────────────────────────────────────────────────────────
float soilRawToPercent(int rawADC) {
  // Inverse mapping: higher ADC = drier (capacitive sensor)
  float pct = (float)(SOIL_DRY_RAW - rawADC) /
              (float)(SOIL_DRY_RAW - SOIL_WET_RAW) * 100.0f;
  if (pct < 0.0f)   pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;
  return pct;
}

// ────────────────────────────────────────────────────────────
//   readPM25 — reads with LED timing as per GP2Y datasheet
//   LED must be ON for exactly 280µs before sampling.
//   Full cycle: pulse ON 320µs → sample at 280µs → OFF → wait 9680µs
// ────────────────────────────────────────────────────────────
static float readPM25Averaged() {
  // Turn LED on (LOW = on for GP2Y active-low)
  digitalWrite(PM25_LED_PIN, LOW);
  delayMicroseconds(280);          // Wait 280µs — datasheet sampling point

  int raw = analogRead(PM25_AOUT_PIN);  // Sample here

  delayMicroseconds(40);           // Hold for remaining 40µs (total pulse = 320µs)
  digitalWrite(PM25_LED_PIN, HIGH); // LED off
  delayMicroseconds(9680);         // Wait rest of 10ms cycle

  float ugm3 = pm25RawToUgm3(raw);

  // Push into circular buffer
  pm25Buffer[pm25BufIdx] = ugm3;
  pm25BufIdx = (pm25BufIdx + 1) % PM25_BUF_SIZE;
  if (pm25BufIdx == 0) pm25BufFull = true;

  // Compute average
  uint8_t count = pm25BufFull ? PM25_BUF_SIZE : pm25BufIdx;
  if (count == 0) return ugm3;
  float sum = 0;
  for (uint8_t i = 0; i < count; i++) sum += pm25Buffer[i];
  return sum / count;
}

// ────────────────────────────────────────────────────────────
//   readSoilAveraged
// ────────────────────────────────────────────────────────────
static float readSoilAveraged() {
  // Take 5 rapid ADC samples and average to reduce noise
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(SOIL_AOUT_PIN);
    delay(10);
  }
  int raw = sum / 5;
  float pct = soilRawToPercent(raw);

  // Moving average
  soilBuffer[soilBufIdx] = pct;
  soilBufIdx = (soilBufIdx + 1) % SOIL_BUF_SIZE;
  if (soilBufIdx == 0) soilBufFull = true;

  uint8_t count = soilBufFull ? SOIL_BUF_SIZE : soilBufIdx;
  if (count == 0) return pct;
  float s = 0;
  for (uint8_t i = 0; i < count; i++) s += soilBuffer[i];
  return s / count;
}

// ────────────────────────────────────────────────────────────
//   sensorsRead — main public function
// ────────────────────────────────────────────────────────────
bool sensorsRead(SensorData &data) {
  data.timestamp = millis();

  // ── DHT11 ─────────────────────────────────────────────────
  float t = dht.readTemperature();  // Celsius
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println(F("[SENSOR] DHT11 read FAILED — check wiring!"));
    data.dhtOk = false;
    // Keep last known values (don't overwrite with NaN)
    // data.temperatureC and data.humidityPct remain unchanged
  } else {
    data.temperatureC = t;
    data.humidityPct  = h;
    data.dhtOk        = true;
  }

  // ── PM2.5 ─────────────────────────────────────────────────
  float pm = readPM25Averaged();
  if (pm >= 0.0f && pm <= 500.0f) {
    data.pm25_ugm3 = pm;
    data.pm25Ok    = true;
  } else {
    Serial.println(F("[SENSOR] PM2.5 read out of range"));
    data.pm25Ok = false;
  }

  // ── Soil Moisture ─────────────────────────────────────────
  float soil = readSoilAveraged();
  if (soil >= 0.0f && soil <= 100.0f) {
    data.moisturePct = soil;
    data.soilOk      = true;
  } else {
    Serial.println(F("[SENSOR] Soil sensor read out of range"));
    data.soilOk = false;
  }

  return (data.dhtOk || data.pm25Ok || data.soilOk);
}

// ────────────────────────────────────────────────────────────
//   sensorsPrint
// ────────────────────────────────────────────────────────────
void sensorsPrint(const SensorData &data) {
  Serial.println(F("──────────────────────────────"));
  Serial.print(F("  Temperature : "));
  Serial.print(data.temperatureC, 1); Serial.println(F(" °C"));
  Serial.print(F("  Humidity    : "));
  Serial.print(data.humidityPct, 1);  Serial.println(F(" % RH"));
  Serial.print(F("  Moisture    : "));
  Serial.print(data.moisturePct, 1);  Serial.println(F(" %"));
  Serial.print(F("  PM2.5       : "));
  Serial.print(data.pm25_ugm3, 1);    Serial.println(F(" µg/m³"));
  Serial.println(F("──────────────────────────────"));
}

// ─── Helper macro for string literal in F() ─────────────────
#define xstr(s) str(s)
#define str(s)  #s
