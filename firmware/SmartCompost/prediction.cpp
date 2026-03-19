// ============================================================
//   prediction.cpp — Compost Maturity Prediction Engine
//
//   Model: Weighted Factor Scoring + Linear Regression Residual
//
//   Health Score = Σ(factor_weight × factor_score) / Σ(weights)
//   Where each factor is scored 0–100 based on how close
//   the measurement is to its optimal range.
//
//   daysRemaining is estimated using:
//     base_days  = MAX_COMPOST_DAYS - (healthScore/100 × 60)
//     adjustment = linear model coefficients × features
//     daysRem    = base_days - daysElapsed + adjustment (clamped ≥ 0)
//
//   Coefficients below were derived from training on a synthetic
//   dataset in ml_model/train_model.py using scikit-learn.
//   Replace with your own trained coefficients after collecting
//   real data for better accuracy.
// ============================================================

#include "prediction.h"
#include "config.h"

// ─── Stage Name Strings ──────────────────────────────────────
const char* STAGE_NAMES[] = {
  "Mesophilic (Warming)",     // 0 — days 0–3, temp 10–40°C
  "Thermophilic (Hot)",       // 1 — days 3–20, temp 40–65°C
  "Cooling",                  // 2 — days 20–35, temp dropping
  "Curing (Maturing)",        // 3 — days 35–55, low activity
  "Ready!"                    // 4 — score ≥ SCORE_READY_THRESHOLD
};

// ─── Linear Regression Coefficients ─────────────────────────
// Trained on synthetic dataset (see ml_model/train_model.py)
// Target: daysRemainingFromOptimal (how many days off from ready)
// Features normalised to [0,1] range before applying coefficients
//
// Formula: adjustment = INTERCEPT
//                     + c_temp    * norm(temp)
//                     + c_hum     * norm(humidity)
//                     + c_moist   * norm(moisture)
//                     + c_pm25    * norm(pm25)
//                     + c_days    * norm(daysElapsed)
//
// Positive coefficient → delays readiness (bad condition)
// Negative coefficient → accelerates readiness (good condition)
static const float LR_INTERCEPT  =  8.50f;
static const float LR_C_TEMP     = -6.20f;   // higher temp (within range) = faster
static const float LR_C_HUM      = -3.10f;   // optimal humidity helps
static const float LR_C_MOIST    = -4.50f;   // optimal moisture accelerates
static const float LR_C_PM25     = -2.80f;   // higher PM2.5 = more active decomp
static const float LR_C_DAYS     = -5.00f;   // more days elapsed = less remaining

// ─── Internal State ─────────────────────────────────────────
static unsigned long _startMillis = 0;

// ────────────────────────────────────────────────────────────
//   factorScore — returns 0–100 score for a single parameter
//   using a trapezoidal membership function:
//
//      100 |        ___________
//           |       /           \
//        0  |______/             \______
//          low_bad low_ok optimal hi_ok hi_bad
// ────────────────────────────────────────────────────────────
static float factorScore(float value,
                         float lowBad, float lowOk,
                         float highOk, float highBad) {
  if (value <= lowBad || value >= highBad) return 0.0f;

  if (value >= lowOk && value <= highOk) return 100.0f;

  if (value > lowBad && value < lowOk) {
    return (value - lowBad) / (lowOk - lowBad) * 100.0f;
  }

  // value > highOk && value < highBad
  return (highBad - value) / (highBad - highOk) * 100.0f;
}

// ────────────────────────────────────────────────────────────
//   normalise — maps value from [minVal, maxVal] → [0, 1]
// ────────────────────────────────────────────────────────────
static float normalise(float value, float minVal, float maxVal) {
  if (maxVal <= minVal) return 0.0f;
  float n = (value - minVal) / (maxVal - minVal);
  if (n < 0.0f) n = 0.0f;
  if (n > 1.0f) n = 1.0f;
  return n;
}

// ────────────────────────────────────────────────────────────
//   detectStage — classify current composting stage
// ────────────────────────────────────────────────────────────
static uint8_t detectStage(float tempC, float healthScore, int daysElapsed) {
  if (healthScore >= SCORE_READY_THRESHOLD && daysElapsed >= 21) return 4;
  if (daysElapsed >= 35)                                          return 3;  // Curing
  if (daysElapsed >= 20 && tempC < 45.0f)                        return 2;  // Cooling
  if (tempC >= 40.0f)                                             return 1;  // Thermophilic
  return 0;                                                                  // Mesophilic
}

// ────────────────────────────────────────────────────────────
//   buildAlerts — generates actionable recommendations
// ────────────────────────────────────────────────────────────
static void buildAlerts(const SensorData &data, String &alerts, String &recommendation) {
  alerts = "";
  recommendation = "All parameters OK. Keep monitoring.";

  // Priority order: most critical first
  if (data.dhtOk) {
    if (data.temperatureC > TEMP_MAX_C) {
      alerts += "TEMP_HIGH|";
      recommendation = "Temperature too high! Turn/aerate compost immediately.";
    } else if (data.temperatureC < TEMP_MIN_C) {
      alerts += "TEMP_LOW|";
      recommendation = "Temperature too low. Add nitrogen-rich material (greens).";
    }

    if (data.humidityPct > HUMIDITY_MAX) {
      alerts += "HUMIDITY_HIGH|";
      if (recommendation.indexOf("OK") != -1)
        recommendation = "Humidity high. Improve ventilation or add dry browns.";
    } else if (data.humidityPct < HUMIDITY_MIN) {
      alerts += "HUMIDITY_LOW|";
      if (recommendation.indexOf("OK") != -1)
        recommendation = "Humidity low. Add water or wet material.";
    }
  }

  if (data.soilOk) {
    if (data.moisturePct > MOISTURE_MAX) {
      alerts += "MOISTURE_WET|";
      if (recommendation.indexOf("OK") != -1)
        recommendation = "Compost too wet! Add dry carbon material (browns/straw).";
    } else if (data.moisturePct < MOISTURE_MIN) {
      alerts += "MOISTURE_DRY|";
      if (recommendation.indexOf("OK") != -1)
        recommendation = "Compost too dry. Add water and turn the pile.";
    }
  }

  if (data.pm25Ok) {
    if (data.pm25_ugm3 > 300.0f) {
      alerts += "PM25_VERY_HIGH|";
      if (recommendation.indexOf("OK") != -1)
        recommendation = "Very high PM2.5 — possible anaerobic zone. Turn pile urgently.";
    }
  }

  // Trim trailing pipe
  if (alerts.length() > 0 && alerts.endsWith("|")) {
    alerts.remove(alerts.length() - 1);
  }

  if (alerts.length() == 0) {
    alerts = "OK";
  }
}

// ────────────────────────────────────────────────────────────
//   predictionInit
// ────────────────────────────────────────────────────────────
void predictionInit(unsigned long startMillis) {
  _startMillis = startMillis;
  Serial.println(F("[PREDICT] Prediction engine initialised."));
}

// ────────────────────────────────────────────────────────────
//   predictionGetDaysElapsed
// ────────────────────────────────────────────────────────────
int predictionGetDaysElapsed() {
  unsigned long elapsed = millis() - _startMillis;
  return (int)(elapsed / 86400000UL);  // ms → days
}

// ────────────────────────────────────────────────────────────
//   predictionRun — main prediction function
// ────────────────────────────────────────────────────────────
void predictionRun(const SensorData &data, int daysElapsed, PredictionResult &result) {

  // ── Step 1: Score each factor (0–100) ─────────────────────
  //
  //  Thresholds reference: Haug (1993), USCC Best Practices (2008)
  //
  float scoreTemp = data.dhtOk
    ? factorScore(data.temperatureC,
                  20.0f,  40.0f,  60.0f,  70.0f)   // bad<20, ok 40-60, bad>70
    : 50.0f;  // assume neutral if sensor failed

  float scoreHum = data.dhtOk
    ? factorScore(data.humidityPct,
                  25.0f,  45.0f,  65.0f,  85.0f)
    : 50.0f;

  float scoreMoist = data.soilOk
    ? factorScore(data.moisturePct,
                  20.0f,  40.0f,  65.0f,  80.0f)
    : 50.0f;

  // PM2.5: Moderate activity (50–200 µg/m³) = active decomp = good.
  // Very low = cold/inactive. Very high = anaerobic gases = bad.
  float scorePm25 = data.pm25Ok
    ? factorScore(data.pm25_ugm3,
                  5.0f,   30.0f,  200.0f, 350.0f)
    : 50.0f;

  // Days progress bonus: more days elapsed (up to 60) = closer to done
  float daysCapped = (float)min(daysElapsed, 60);
  float scoreDays  = (daysCapped / 60.0f) * 100.0f;

  // ── Step 2: Weighted health score ─────────────────────────
  //  Weights reflect importance of each parameter to composting
  const float W_TEMP  = 0.30f;
  const float W_HUM   = 0.15f;
  const float W_MOIST = 0.25f;
  const float W_PM25  = 0.15f;
  const float W_DAYS  = 0.15f;

  result.healthScore = (scoreTemp  * W_TEMP  +
                        scoreHum   * W_HUM   +
                        scoreMoist * W_MOIST +
                        scorePm25  * W_PM25  +
                        scoreDays  * W_DAYS);

  // Clamp
  if (result.healthScore < 0.0f)   result.healthScore = 0.0f;
  if (result.healthScore > 100.0f) result.healthScore = 100.0f;

  // ── Step 3: Linear regression adjustment ──────────────────
  //  Normalise each feature to [0,1]
  float nTemp  = normalise(data.temperatureC, 20.0f, 70.0f);
  float nHum   = normalise(data.humidityPct,  25.0f, 85.0f);
  float nMoist = normalise(data.moisturePct,  20.0f, 80.0f);
  float nPm25  = normalise(data.pm25_ugm3,    0.0f,  350.0f);
  float nDays  = normalise((float)daysElapsed, 0.0f, 60.0f);

  float lrAdjustment = LR_INTERCEPT
                     + LR_C_TEMP  * nTemp
                     + LR_C_HUM   * nHum
                     + LR_C_MOIST * nMoist
                     + LR_C_PM25  * nPm25
                     + LR_C_DAYS  * nDays;

  // ── Step 4: Estimate days remaining ───────────────────────
  //  Base estimate: perfectly conditioned compost → 21 days
  //                 worst conditioned → 90 days
  //  Scale by inverse health score
  float baseRemaining = MAX_COMPOST_DAYS -
                        (result.healthScore / 100.0f) * (MAX_COMPOST_DAYS - MIN_COMPOST_DAYS);

  // Subtract days already elapsed and apply LR adjustment
  float rawRemaining = baseRemaining - (float)daysElapsed + lrAdjustment;

  // Hard rules: if already past expected duration with good score → nearly ready
  if (daysElapsed >= 45 && result.healthScore >= 70.0f) {
    rawRemaining = min(rawRemaining, 7.0f);  // at most 1 week
  }

  result.daysRemaining = (int)max(0.0f, rawRemaining);

  // ── Step 5: Stage detection ────────────────────────────────
  result.stage = detectStage(data.temperatureC, result.healthScore, daysElapsed);

  // ── Step 6: Status string ─────────────────────────────────
  if (result.stage == 4 || result.daysRemaining == 0) {
    result.status = "Ready!";
  } else if (result.healthScore >= SCORE_GOOD_THRESHOLD) {
    result.status = "Good Progress";
  } else if (result.healthScore >= 40.0f) {
    result.status = "In Progress";
  } else {
    result.status = "Needs Attention";
  }

  // ── Step 7: Alerts & recommendations ─────────────────────
  buildAlerts(data, result.alerts, result.recommendation);
}

// ────────────────────────────────────────────────────────────
//   predictionPrint
// ────────────────────────────────────────────────────────────
void predictionPrint(const PredictionResult &result) {
  Serial.println(F("====  PREDICTION RESULT  ===="));
  Serial.print  (F("  Stage       : ")); Serial.println(STAGE_NAMES[result.stage]);
  Serial.print  (F("  Status      : ")); Serial.println(result.status);
  Serial.print  (F("  Health Score: ")); Serial.println(result.healthScore, 1);
  Serial.print  (F("  Days Remain : ")); Serial.println(result.daysRemaining);
  Serial.print  (F("  Alerts      : ")); Serial.println(result.alerts);
  Serial.print  (F("  Action      : ")); Serial.println(result.recommendation);
  Serial.println(F("============================="));
}
