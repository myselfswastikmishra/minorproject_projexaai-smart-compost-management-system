#ifndef SENSORS_H
#define SENSORS_H

// ============================================================
//   sensors.h — Sensor Abstraction Layer
//   Handles DHT11, PM2.5 (GP2Y1010AU0F), Soil Moisture
// ============================================================

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

// ─── Sensor Data Structure ───────────────────────────────────
struct SensorData {
  float  temperatureC;     // °C from DHT11
  float  humidityPct;      // % RH from DHT11
  float  moisturePct;      // % volumetric moisture from soil sensor
  float  pm25_ugm3;        // µg/m³ from PM2.5 GP2Y
  bool   dhtOk;            // true if DHT11 read was valid
  bool   pm25Ok;           // true if PM2.5 read was valid
  bool   soilOk;           // true if soil read was valid
  unsigned long timestamp; // millis() at time of reading
};

// ─── Public API ─────────────────────────────────────────────

/**
 * Call once in setup() to initialise all sensors.
 */
void sensorsInit();

/**
 * Read all sensors and populate a SensorData struct.
 * Handles PM2.5 LED timing internally (sampling window = 280µs).
 * Returns true if at least one sensor returned valid data.
 */
bool sensorsRead(SensorData &data);

/**
 * Pretty-print sensor data to Serial.
 */
void sensorsPrint(const SensorData &data);

// ─── Internal helpers (exposed for unit testing) ────────────
float pm25RawToUgm3(int rawADC);
float soilRawToPercent(int rawADC);

#endif // SENSORS_H
