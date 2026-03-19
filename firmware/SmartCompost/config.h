#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//   SmartCompost IoT — Configuration
//   Author : Swastik Mishra | Roll: 2401010019
//   Project: IoT-Based Smart Composting System
//   Board  : ESP32 (38-pin or 30-pin dev module)
// ============================================================

// ─── WiFi Credentials ───────────────────────────────────────
// Set these or define via Arduino IoT Cloud Secret tab
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

// ─── Arduino IoT Cloud ─────────────────────────────────────
// Copy from your Arduino IoT Cloud Thing > Setup tab
#define DEVICE_ID       "YOUR_DEVICE_ID_HERE"
#define SECRET_KEY      "YOUR_SECRET_KEY_HERE"

// ─── Pin Definitions ────────────────────────────────────────
// DHT11 — Temperature & Humidity
#define DHT_PIN         4         // GPIO4  — data pin of DHT11
#define DHT_TYPE        DHT11

// PM2.5 Sharp GP2Y1010AU0F — Dust/Air Quality Sensor
#define PM25_LED_PIN    2         // GPIO2  — drives infrared LED (via 150Ω + 220µF)
#define PM25_AOUT_PIN   34        // GPIO34 — analog output (ADC1, input-only pin)

// Soil Moisture Sensor (Capacitive v1.2 or resistive)
// Measures moisture % inside the compost bin — critical for decomposition
#define SOIL_AOUT_PIN   35        // GPIO35 — analog output (ADC1, input-only pin)

// Optional: Buzzer / LED indicator for "Compost Ready" alert
#define BUZZER_PIN      5         // GPIO5  — active buzzer or LED
#define STATUS_LED_PIN  18        // GPIO18 — RGB or single LED

// ─── Sensor Calibration ─────────────────────────────────────
// PM2.5 — adjust Vcc and Vref for your ESP32 board
#define PM25_VCC        3.3f      // ESP32 ADC reference voltage
#define PM25_ADC_MAX    4095      // 12-bit ADC

// Soil Moisture — calibrate by measuring raw ADC in air (dry) and in water (wet)
#define SOIL_DRY_RAW    3200      // ADC reading in completely dry air
#define SOIL_WET_RAW    1100      // ADC reading fully submerged in water

// ─── Composting Thresholds ──────────────────────────────────
// Based on standard aerobic composting research (Haug, 1993)
#define TEMP_MIN_C      40.0f     // Minimum ideal temp for thermophilic composting
#define TEMP_MAX_C      65.0f     // Above this → too hot, microbes die
#define TEMP_OPTIMAL_C  55.0f     // Sweet spot for fastest decomposition

#define HUMIDITY_MIN    40.0f     // % RH — below this, microbes slow down
#define HUMIDITY_MAX    70.0f     // % RH — above this, anaerobic risk
#define HUMIDITY_OPT    55.0f     // % RH — optimal

#define MOISTURE_MIN    40.0f     // % volumetric moisture in compost
#define MOISTURE_MAX    65.0f     // % — soggy = anaerobic = bad smell
#define MOISTURE_OPT    55.0f     // % — optimal

#define PM25_ACTIVE     100.0f    // µg/m³ — above = active decomp microbial gases
#define PM25_READY      30.0f     // µg/m³ — below this during cool phase = nearing ready

// ─── Composting Stage Classification ────────────────────────
// Score: 0–100 assigned internally; 100 = perfectly conditioned
#define SCORE_READY_THRESHOLD  80   // health score above this → "Compost Ready"
#define SCORE_GOOD_THRESHOLD   60   // score above this → "Good Progress"

// ─── Sampling Intervals ─────────────────────────────────────
#define SENSOR_READ_INTERVAL_MS   10000UL   // Read sensors every 10 seconds
#define CLOUD_SYNC_INTERVAL_MS    30000UL   // Sync to Arduino IoT Cloud every 30s
#define PREDICTION_INTERVAL_MS    60000UL   // Re-run prediction model every 60s

// ─── Compost Duration Bounds (days) ─────────────────────────
#define MIN_COMPOST_DAYS   14     // Hot composting minimum
#define MAX_COMPOST_DAYS   90     // Cold / neglected composting max

#endif // CONFIG_H
