"""
train_model.py — Compost Maturity Prediction Model
====================================================
Author  : Swastik Mishra | Roll: 2401010019
Project : IoT-Based Smart Composting System

This script:
  1. Generates a synthetic training dataset based on
     composting science (Haug 1993, USCC guidelines)
  2. Trains a Linear Regression + Random Forest model
  3. Evaluates accuracy (MAE, R²)
  4. Exports coefficients as a C header file for ESP32
  5. Saves the trained model as a .pkl for future use

Run:
  pip install -r requirements.txt
  python train_model.py

Output:
  model_output/compost_model.pkl
  model_output/model_coefficients.h   ← copy to firmware/SmartCompost/
  model_output/feature_importance.png
  model_output/prediction_vs_actual.png
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import joblib
import os
from sklearn.linear_model import LinearRegression, Ridge
from sklearn.ensemble import RandomForestRegressor, GradientBoostingRegressor
from sklearn.model_selection import train_test_split, cross_val_score
from sklearn.metrics import mean_absolute_error, r2_score, mean_squared_error
from sklearn.preprocessing import MinMaxScaler
from sklearn.pipeline import Pipeline

# ─── Output directory ────────────────────────────────────────
os.makedirs("model_output", exist_ok=True)

# ════════════════════════════════════════════════════════════
#   1. SYNTHETIC DATASET GENERATION
#   Simulates realistic composting scenarios across 90 days
# ════════════════════════════════════════════════════════════

np.random.seed(42)
N_SAMPLES = 3000

def generate_composting_dataset(n=3000):
    """
    Generate synthetic sensor readings for composting scenarios.
    
    Composting stages (Haug 1993):
      Day  0–3  : Mesophilic (temp rises 25→40°C)
      Day  3–20 : Thermophilic (temp 40–65°C, high activity)
      Day 20–35 : Cooling (temp drops, activity slows)
      Day 35–55 : Curing / Maturing (stabilisation)
      Day 55+   : Potentially ready (if conditions were good)
    
    Features:
      temperature_c  — DHT11
      humidity_pct   — DHT11
      moisture_pct   — Soil sensor
      pm25_ugm3      — PM2.5 sensor
      days_elapsed   — Counter since start
    
    Target:
      days_remaining — How many more days until compost is ready
    """
    
    records = []
    
    for _ in range(n):
        # Random day in composting cycle
        days_elapsed = np.random.randint(0, 80)
        
        # Determine base conditions by stage
        if days_elapsed < 3:   # Mesophilic warm-up
            temp_mean, temp_std = 30.0, 6.0
            pm25_mean = 40.0
        elif days_elapsed < 20:  # Thermophilic
            temp_mean, temp_std = 52.0, 8.0
            pm25_mean = 150.0
        elif days_elapsed < 35:  # Cooling
            temp_mean, temp_std = 38.0, 7.0
            pm25_mean = 80.0
        else:                    # Curing
            temp_mean, temp_std = 27.0, 5.0
            pm25_mean = 30.0
        
        # Sample with noise — simulates real-world variation
        temperature = np.clip(np.random.normal(temp_mean, temp_std), 15.0, 75.0)
        humidity    = np.clip(np.random.normal(58.0, 12.0), 20.0, 90.0)
        moisture    = np.clip(np.random.normal(52.0, 10.0), 15.0, 85.0)
        pm25        = np.clip(np.random.normal(pm25_mean, pm25_mean * 0.3), 5.0, 400.0)
        
        # ── Compute "ideal" days for this scenario ─────────────
        # Perfect conditions = 21 days (hot composting)
        # Poor conditions    = 80+ days
        # Each deviation from optimal adds days
        
        # Temperature penalty (0 = perfect 55°C)
        temp_dev   = abs(temperature - 55.0)
        temp_pen   = temp_dev * 0.5           # 1°C off = +0.5 days

        # Humidity penalty (optimal 55%)
        hum_dev    = abs(humidity - 55.0)
        hum_pen    = hum_dev * 0.3

        # Moisture penalty (optimal 55%)
        moist_dev  = abs(moisture - 55.0)
        moist_pen  = moist_dev * 0.35

        # PM2.5 — low activity = slower (optimal ~120)
        pm25_dev   = abs(pm25 - 120.0)
        pm25_pen   = pm25_dev * 0.04

        # Base: minimum 21 days for perfect conditions
        base_days  = 21.0
        total_days = base_days + temp_pen + hum_pen + moist_pen + pm25_pen
        total_days = np.clip(total_days, 21.0, 90.0)
        
        # Days remaining = total needed - days elapsed
        days_remaining = max(0, total_days - days_elapsed)
        
        # Add small noise to target
        days_remaining += np.random.normal(0, 1.5)
        days_remaining  = max(0.0, days_remaining)
        
        records.append({
            'temperature_c' : round(temperature, 2),
            'humidity_pct'  : round(humidity, 2),
            'moisture_pct'  : round(moisture, 2),
            'pm25_ugm3'     : round(pm25, 2),
            'days_elapsed'  : int(days_elapsed),
            'days_remaining': round(days_remaining, 1)
        })
    
    return pd.DataFrame(records)


print("=" * 60)
print("  SmartCompost — ML Model Training")
print("=" * 60)

print("\n[1/5] Generating synthetic dataset...")
df = generate_composting_dataset(N_SAMPLES)
df.to_csv("model_output/dataset.csv", index=False)
print(f"      Generated {len(df)} samples → model_output/dataset.csv")
print(df.describe().round(2))


# ════════════════════════════════════════════════════════════
#   2. FEATURE ENGINEERING & SPLITTING
# ════════════════════════════════════════════════════════════

print("\n[2/5] Preparing features...")

FEATURES = ['temperature_c', 'humidity_pct', 'moisture_pct', 'pm25_ugm3', 'days_elapsed']
TARGET   = 'days_remaining'

X = df[FEATURES].values
y = df[TARGET].values

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42
)
print(f"      Train: {len(X_train)} | Test: {len(X_test)}")


# ════════════════════════════════════════════════════════════
#   3. TRAIN MODELS
# ════════════════════════════════════════════════════════════

print("\n[3/5] Training models...")

scaler = MinMaxScaler()

# ── Model A: Linear Regression (for ESP32 coefficient export) ──
lr_pipeline = Pipeline([
    ('scaler', MinMaxScaler()),
    ('model',  LinearRegression())
])
lr_pipeline.fit(X_train, y_train)

lr_preds = lr_pipeline.predict(X_test)
lr_mae   = mean_absolute_error(y_test, lr_preds)
lr_r2    = r2_score(y_test, lr_preds)
print(f"      Linear Regression  → MAE: {lr_mae:.2f} days | R²: {lr_r2:.3f}")

# ── Model B: Random Forest (better accuracy, for reference) ─────
rf_pipeline = Pipeline([
    ('scaler', MinMaxScaler()),
    ('model',  RandomForestRegressor(n_estimators=100, max_depth=8,
                                      random_state=42, n_jobs=-1))
])
rf_pipeline.fit(X_train, y_train)

rf_preds = rf_pipeline.predict(X_test)
rf_mae   = mean_absolute_error(y_test, rf_preds)
rf_r2    = r2_score(y_test, rf_preds)
print(f"      Random Forest      → MAE: {rf_mae:.2f} days | R²: {rf_r2:.3f}")

# ── Model C: Gradient Boosting (best accuracy) ───────────────────
gb_pipeline = Pipeline([
    ('scaler', MinMaxScaler()),
    ('model',  GradientBoostingRegressor(n_estimators=150, max_depth=4,
                                          learning_rate=0.1, random_state=42))
])
gb_pipeline.fit(X_train, y_train)

gb_preds = gb_pipeline.predict(X_test)
gb_mae   = mean_absolute_error(y_test, gb_preds)
gb_r2    = r2_score(y_test, gb_preds)
print(f"      Gradient Boosting  → MAE: {gb_mae:.2f} days | R²: {gb_r2:.3f}")

# Save best model (Random Forest)
joblib.dump(rf_pipeline, "model_output/compost_model.pkl")
print("\n      Best model saved → model_output/compost_model.pkl")


# ════════════════════════════════════════════════════════════
#   4. EXPORT LINEAR REGRESSION COEFFICIENTS FOR ESP32
# ════════════════════════════════════════════════════════════

print("\n[4/5] Exporting coefficients for ESP32...")

lr_model     = lr_pipeline.named_steps['model']
lr_scaler    = lr_pipeline.named_steps['scaler']
coefs        = lr_model.coef_
intercept    = lr_model.intercept_

print(f"\n  Linear Regression Coefficients:")
print(f"  Intercept         : {intercept:.4f}")
for feat, coef in zip(FEATURES, coefs):
    print(f"  {feat:<20}: {coef:+.4f}")

# Write C header for ESP32
header_content = f"""// ============================================================
//   model_coefficients.h — Auto-generated by train_model.py
//   Copy this file to firmware/SmartCompost/
//   Linear Regression model for compost days-remaining prediction
//
//   Model: Linear Regression (MAE = {lr_mae:.2f} days, R² = {lr_r2:.3f})
//   Trained on {N_SAMPLES} synthetic samples
//   Features normalised to [0,1] using MinMaxScaler
//
//   Normalisation ranges (from training data):
//   Feature           Min      Max
"""

for i, feat in enumerate(FEATURES):
    header_content += f"//   {feat:<22} {lr_scaler.data_min_[i]:6.2f}   {lr_scaler.data_max_[i]:6.2f}\n"

header_content += f"""// ============================================================

#ifndef MODEL_COEFFICIENTS_H
#define MODEL_COEFFICIENTS_H

// ─── Scaler ranges (min, max for each feature) ─────────────
static const float FEAT_MIN[] = {{{', '.join([f"{v:.4f}f" for v in lr_scaler.data_min_])}}};
static const float FEAT_MAX[] = {{{', '.join([f"{v:.4f}f" for v in lr_scaler.data_max_])}}};

// ─── Linear Regression Coefficients ────────────────────────
static const float MODEL_INTERCEPT = {intercept:.4f}f;
static const float MODEL_COEFS[]   = {{
  {coefs[0]:+.4f}f,  // temperature_c
  {coefs[1]:+.4f}f,  // humidity_pct
  {coefs[2]:+.4f}f,  // moisture_pct
  {coefs[3]:+.4f}f,  // pm25_ugm3
  {coefs[4]:+.4f}f   // days_elapsed
}};
#define N_FEATURES 5

// ─── Prediction helper (include this header, call this fn) ─
inline float predictDaysRemaining(float tempC, float humPct,
                                   float moistPct, float pm25,
                                   int   daysElapsed) {{
  float features[N_FEATURES] = {{tempC, humPct, moistPct, pm25, (float)daysElapsed}};
  float result = MODEL_INTERCEPT;
  for (int i = 0; i < N_FEATURES; i++) {{
    float range = FEAT_MAX[i] - FEAT_MIN[i];
    float norm  = (range > 0.0f) ? (features[i] - FEAT_MIN[i]) / range : 0.0f;
    norm = (norm < 0.0f) ? 0.0f : (norm > 1.0f) ? 1.0f : norm;
    result += MODEL_COEFS[i] * norm;
  }}
  return (result < 0.0f) ? 0.0f : result;
}}

#endif // MODEL_COEFFICIENTS_H
"""

with open("model_output/model_coefficients.h", "w") as f:
    f.write(header_content)

print("      Exported → model_output/model_coefficients.h")


# ════════════════════════════════════════════════════════════
#   5. PLOTS
# ════════════════════════════════════════════════════════════

print("\n[5/5] Generating plots...")

fig, axes = plt.subplots(1, 3, figsize=(18, 5))
fig.suptitle("SmartCompost — ML Model Evaluation", fontsize=14, fontweight="bold")

# ── Plot 1: Prediction vs Actual ─────────────────────────────
for ax, preds, title, color in zip(
    axes,
    [lr_preds, rf_preds, gb_preds],
    ["Linear Regression", "Random Forest", "Gradient Boosting"],
    ["#2ECC71", "#3498DB", "#E67E22"]
):
    ax.scatter(y_test, preds, alpha=0.4, s=10, color=color)
    ax.plot([0, 90], [0, 90], 'k--', linewidth=1, label="Perfect prediction")
    ax.set_xlabel("Actual Days Remaining")
    ax.set_ylabel("Predicted Days Remaining")
    ax.set_title(f"{title}\nMAE={mean_absolute_error(y_test, preds):.2f}d  R²={r2_score(y_test, preds):.3f}")
    ax.legend(fontsize=8)
    ax.set_xlim(0, 90)
    ax.set_ylim(0, 90)
    ax.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig("model_output/prediction_vs_actual.png", dpi=150, bbox_inches="tight")
print("      Saved → model_output/prediction_vs_actual.png")

# ── Feature Importance (Random Forest) ───────────────────────
fig2, ax2 = plt.subplots(figsize=(8, 4))
rf_model = rf_pipeline.named_steps['model']
importances = rf_model.feature_importances_
colors = ['#2ECC71', '#3498DB', '#E67E22', '#9B59B6', '#E74C3C']

bars = ax2.barh(FEATURES, importances, color=colors)
ax2.set_xlabel("Feature Importance")
ax2.set_title("Random Forest — Feature Importance\n(Higher = More Influence on Prediction)")
ax2.invert_yaxis()
for bar, val in zip(bars, importances):
    ax2.text(bar.get_width() + 0.002, bar.get_y() + bar.get_height()/2,
             f"{val:.3f}", va='center', fontsize=9)

plt.tight_layout()
plt.savefig("model_output/feature_importance.png", dpi=150, bbox_inches="tight")
print("      Saved → model_output/feature_importance.png")
plt.show()

# ════════════════════════════════════════════════════════════
#   SUMMARY
# ════════════════════════════════════════════════════════════

print("\n" + "=" * 60)
print("  TRAINING COMPLETE — SUMMARY")
print("=" * 60)
print(f"  Dataset          : {N_SAMPLES} samples")
print(f"  Linear Regression: MAE = {lr_mae:.2f} days  | R² = {lr_r2:.4f}")
print(f"  Random Forest    : MAE = {rf_mae:.2f} days  | R² = {rf_r2:.4f}")
print(f"  Gradient Boosting: MAE = {gb_mae:.2f} days  | R² = {gb_r2:.4f}")
print()
print("  Files generated:")
print("    model_output/dataset.csv")
print("    model_output/compost_model.pkl         ← main model")
print("    model_output/model_coefficients.h      ← copy to ESP32 firmware")
print("    model_output/prediction_vs_actual.png")
print("    model_output/feature_importance.png")
print()
print("  Next steps:")
print("    1. Copy model_coefficients.h to firmware/SmartCompost/")
print("    2. In prediction.cpp, use predictDaysRemaining() from that header")
print("    3. Collect real sensor data and retrain for better accuracy")
print("=" * 60)
