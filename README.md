# 🌱 SmartCompost IoT

> **IoT-Based Smart Composting System for Sustainable Organic Waste Management**
> Real-time monitoring of bio-waste decomposition using ESP32 + sensor fusion + ML prediction

---

## 📋 Project Overview

| Field | Details |
|-------|---------|
| **Student** | Swastik Mishra |
| **Roll No** | 2401010019 |
| **Program** | B.Tech Computer Science & Engineering |
| **Year** | 2nd Year (2024–28) |
| **University** | K.R. Mangalam University, Gurgaon |
| **Mentor** | Dr. Manish Kumar |
| **Status** | Mid-Term Review |

---

## 🎯 What This System Does

Traditional composting requires constant manual monitoring — checking temperature, moisture, and smell every day. This project automates that completely:

1. **Three sensors** embedded in or near the compost bin continuously measure environmental parameters
2. **ESP32** reads all sensors every 10 seconds and processes the data
3. **Arduino IoT Cloud** receives data and displays a live dashboard
4. **ML prediction model** estimates how many more days until the bio-waste has turned into usable compost
5. **Buzzer alert** sounds when compost is ready — no guesswork needed

---

## 🔧 Hardware Components

### Why These Three Sensors?

| Sensor | What It Measures | Why It Matters for Composting |
|--------|-----------------|-------------------------------|
| **DHT11** | Temperature (°C) + Humidity (% RH) | Temperature is THE #1 indicator of decomposition activity. Thermophilic bacteria (40–65°C) break down organic matter 5× faster than cold conditions. Humidity affects microbial metabolism. |
| **GP2Y1010AU0F PM2.5** | Air particulate density (µg/m³) | Rising PM2.5 = active microbial gas release (CO₂, NH₃) = proof of decomposition. Drops when compost cools and stabilises = ready phase. |
| **Capacitive Soil Moisture Sensor v1.2** | Moisture % in compost material | Optimal compost moisture is 40–60%. Too dry = microbes die. Too wet = anaerobic conditions → bad smell. This is the most actionable parameter to control. |

Together, these three sensors give a complete picture of the "five pillars" of composting:
- ✅ Temperature (DHT11)
- ✅ Moisture (Soil sensor)
- ✅ Aeration proxy (PM2.5)
- ✅ Ambient humidity (DHT11)
- ✅ Time elapsed (internal counter)

---

## 📁 Project Structure

```
SmartCompost-IoT/
│
├── firmware/
│   └── SmartCompost/
│       ├── SmartCompost.ino       ← Main Arduino sketch (upload this)
│       ├── thingProperties.h      ← Arduino IoT Cloud variables
│       ├── config.h               ← Pins, WiFi, thresholds (edit this first)
│       ├── sensors.h              ← Sensor abstraction header
│       ├── sensors.cpp            ← DHT11, PM2.5, Soil implementations
│       ├── prediction.h           ← Prediction engine header
│       └── prediction.cpp         ← Composting model implementation
│
├── ml_model/
│   ├── train_model.py             ← Python ML training script
│   ├── requirements.txt           ← pip dependencies
│   └── model_output/              ← Generated after running train_model.py
│       ├── compost_model.pkl
│       ├── model_coefficients.h   ← Copy to firmware/ after training
│       ├── dataset.csv
│       ├── prediction_vs_actual.png
│       └── feature_importance.png
│
├── docs/
│   └── circuit_diagram.md         ← Full wiring guide + ASCII diagram
│
├── dashboard/
│   └── DASHBOARD_SETUP.md         ← Arduino IoT Cloud dashboard guide
│
└── README.md                      ← This file
```

---

## ⚡ Quick Start

### Step 1: Wire the Hardware

Follow the detailed wiring guide in [`docs/circuit_diagram.md`](docs/circuit_diagram.md).

**Pin summary:**
```
DHT11  DATA → GPIO 4
PM2.5  LED  → GPIO 2 (via 150Ω resistor + 220µF cap)
PM2.5  Vout → GPIO 34 (analog)
Soil   AOUT → GPIO 35 (analog)
Buzzer      → GPIO 5
Status LED  → GPIO 18 (via 220Ω)
```

### Step 2: Configure Credentials

Open `firmware/SmartCompost/config.h` and set:
```c
#define WIFI_SSID       "your_wifi_name"
#define WIFI_PASSWORD   "your_wifi_password"
#define DEVICE_ID       "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
#define SECRET_KEY      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
```
Get DEVICE_ID and SECRET_KEY from [cloud.arduino.cc](https://cloud.arduino.cc).

### Step 3: Install Arduino Libraries

In Arduino IDE → Library Manager, install:
- `ArduinoIoTCloud` by Arduino
- `Arduino_ConnectionHandler` by Arduino
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor` by Adafruit

Board: **ESP32 Dev Module** via `esp32` by Espressif board package.

### Step 4: Upload Sketch

1. Open `firmware/SmartCompost/SmartCompost.ino`
2. Select board: **ESP32 Dev Module**
3. Select correct COM port
4. Upload
5. Open Serial Monitor at **115200 baud**

### Step 5: Set Up Cloud Dashboard

Follow [`dashboard/DASHBOARD_SETUP.md`](dashboard/DASHBOARD_SETUP.md) to create the Arduino IoT Cloud dashboard with live gauges, status cards, and trend widgets.

### Step 6 (Optional): Train the ML Model

```bash
cd ml_model
pip install -r requirements.txt
python train_model.py
# Copy model_output/model_coefficients.h → firmware/SmartCompost/
```

---

## 📊 Prediction Model

The system uses a **multi-factor weighted scoring + linear regression** approach:

### Health Score (0–100)
Each sensor reading is scored against optimal composting ranges using a trapezoidal membership function, then combined with weights:

| Factor | Weight | Optimal Range |
|--------|--------|---------------|
| Temperature | 30% | 40–60°C |
| Moisture | 25% | 40–65% |
| Humidity | 15% | 45–65% RH |
| PM2.5 Activity | 15% | 30–200 µg/m³ |
| Days Elapsed | 15% | (progress bonus) |

### Compost Stages

| Stage | Typical Days | Temperature | What's Happening |
|-------|-------------|-------------|-----------------|
| Mesophilic | 0–3 | 25–40°C | Initial microbial colonisation |
| Thermophilic | 3–20 | 40–65°C | Peak decomposition, pathogens killed |
| Cooling | 20–35 | 30–45°C | Activity slows, fungi take over |
| Curing | 35–55 | 25–30°C | Stabilisation, humus formation |
| Ready! | 55+ | Near ambient | Dark, crumbly, earthy smell |

### Days-Remaining Formula
```
base_days  = 90 - (healthScore/100 × 69)
adjustment = LR_model(normalised_features)
remaining  = max(0, base_days - days_elapsed + adjustment)
```

---

## 🖥️ Dashboard Variables

| Variable | Type | Description |
|----------|------|-------------|
| `temperature` | Float | DHT11 temperature in °C |
| `humidity` | Float | DHT11 relative humidity % |
| `moisture` | Float | Soil sensor moisture % |
| `pm25` | Float | Air particulate density µg/m³ |
| `healthScore` | Float | Composite condition score 0–100 |
| `daysElapsed` | Int | Days since monitoring started |
| `daysRemaining` | Int | Predicted days until compost ready |
| `compostStatus` | String | Current status label |
| `alerts` | String | Active warning codes |
| `buzzerEnabled` | Bool | Toggle buzzer from cloud (read/write) |

---

## 🚨 Alert Codes

| Code | Meaning | Action |
|------|---------|--------|
| `TEMP_HIGH` | Temperature > 65°C | Turn/aerate pile immediately |
| `TEMP_LOW` | Temperature < 40°C | Add nitrogen-rich material (greens) |
| `HUMIDITY_HIGH` | Humidity > 70% RH | Improve ventilation |
| `HUMIDITY_LOW` | Humidity < 40% RH | Add wet material |
| `MOISTURE_WET` | Moisture > 65% | Add dry browns (straw, cardboard) |
| `MOISTURE_DRY` | Moisture < 40% | Add water and turn pile |
| `PM25_VERY_HIGH` | PM2.5 > 300 µg/m³ | Possible anaerobic zone — turn urgently |
| `OK` | All parameters nominal | Keep monitoring |

---

## 🔬 Calibration Guide

### Soil Moisture Sensor
1. Run the sketch and open Serial Monitor
2. **Dry calibration**: Leave sensor in open air, note ADC value → set `SOIL_DRY_RAW`
3. **Wet calibration**: Submerge sensor tip in water, note ADC value → set `SOIL_WET_RAW`
4. Typical values: Dry ≈ 3200, Wet ≈ 1100

### PM2.5 Sensor (GP2Y1010AU0F)
1. In clean air, the baseline reading should be 0–20 µg/m³
2. If reading is negative or very high in clean air, adjust `Voc` in `sensors.cpp`
3. For 5V operation, use a voltage divider on Vout pin (10kΩ + 20kΩ)

---

## 📈 Future Enhancements (Planned)

- [ ] MQ-4 methane sensor for anaerobic detection
- [ ] Random Forest model replacing linear regression (after real data collection)
- [ ] Mobile push notifications via Firebase Cloud Messaging
- [ ] Docker-based web dashboard alternative to Arduino IoT Cloud
- [ ] 7-day historical trend charts
- [ ] Multi-bin support

---

## 📚 References

1. Haug, R.T. (1993). *The Practical Handbook of Compost Engineering*. Lewis Publishers.
2. USCC Best Management Practices for Compost Producers (2008).
3. ESP32 Technical Reference Manual — Espressif Systems.
4. Sharp GP2Y1010AU0F Datasheet (Application Note AN-PSD-003).
5. Arduino IoT Cloud Documentation — [docs.arduino.cc](https://docs.arduino.cc/arduino-cloud/)

---

## 📄 License

MIT License — free to use for academic and personal projects.

---

*Built for K.R. Mangalam University Mid-Term Review | Academic Year 2024–28*
