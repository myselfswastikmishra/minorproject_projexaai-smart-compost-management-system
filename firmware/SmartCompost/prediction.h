#ifndef PREDICTION_H
#define PREDICTION_H

// ============================================================
//   prediction.h — Compost Maturity Prediction Engine
//
//   Uses a multi-factor weighted scoring model derived from
//   composting research (Haug 1993, USCC guidelines).
//   Combines rule-based heuristics + linear regression
//   coefficients (exported from Python model in ml_model/).
//
//   Outputs:
//     • healthScore   (0–100)  — current compost condition
//     • daysRemaining (int)    — estimated days until ready
//     • compostStatus (string) — human-readable stage
//     • alerts        (string) — actionable recommendations
// ============================================================

#include <Arduino.h>
#include "sensors.h"

// ─── Prediction Result Structure ────────────────────────────
struct PredictionResult {
  float   healthScore;       // 0–100 composite condition score
  int     daysRemaining;     // estimated days to compost readiness
  String  status;            // "Initializing" / "Active Decomp" / "Maturing" / "Ready!"
  String  alerts;            // Pipe-separated alert messages
  String  recommendation;    // Single most important action to take
  uint8_t stage;             // 0=Mesophilic 1=Thermophilic 2=Cooling 3=Curing 4=Ready
};

// ─── Composting Stage Labels ─────────────────────────────────
extern const char* STAGE_NAMES[];

// ─── Public API ─────────────────────────────────────────────

/**
 * Initialise prediction engine. Call once in setup().
 * @param startDay   millis() when monitoring began (use 0 for "now")
 */
void predictionInit(unsigned long startMillis);

/**
 * Run the prediction model on fresh sensor data.
 * @param data        Latest sensor readings
 * @param daysElapsed Days since monitoring started
 * @param result      Output struct (populated by this function)
 */
void predictionRun(const SensorData &data, int daysElapsed, PredictionResult &result);

/**
 * Pretty-print prediction result to Serial.
 */
void predictionPrint(const PredictionResult &result);

/**
 * Returns days elapsed since predictionInit() was called.
 */
int predictionGetDaysElapsed();

#endif // PREDICTION_H
