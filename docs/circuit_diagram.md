# Circuit Diagram & Wiring Guide
**SmartCompost IoT — ESP32 + DHT11 + PM2.5 + Soil Moisture Sensor**

---

## Component List

| # | Component | Qty | Purpose |
|---|-----------|-----|---------|
| 1 | ESP32 Dev Board (38-pin) | 1 | Microcontroller + Wi-Fi |
| 2 | DHT11 Sensor Module | 1 | Temperature + Humidity |
| 3 | Sharp GP2Y1010AU0F | 1 | PM2.5 / Air Quality |
| 4 | Capacitive Soil Moisture Sensor v1.2 | 1 | Compost moisture % |
| 5 | Active Buzzer Module | 1 | Ready alert |
| 6 | LED (Green, 5mm) | 1 | Status indicator |
| 7 | Resistor 150Ω | 1 | PM2.5 LED current limit |
| 8 | Capacitor 220µF / 10V | 1 | PM2.5 LED power filter |
| 9 | Resistor 220Ω | 1 | Status LED current limit |
| 10 | Breadboard + Jumper Wires | — | Prototyping |
| 11 | Micro USB cable | 1 | Programming + Power |

---

## Pin Connections

### DHT11 → ESP32
```
DHT11 Pin   →   ESP32 Pin
─────────────────────────
VCC   (1)   →   3.3V
DATA  (2)   →   GPIO 4
NC    (3)   →   — (not connected)
GND   (4)   →   GND

Note: Add a 4.7kΩ pull-up resistor between DATA and VCC
      if using a bare DHT11 (not pre-soldered module).
```

### Sharp GP2Y1010AU0F PM2.5 → ESP32
```
GP2Y Pin    Label   →   ESP32 / Component
──────────────────────────────────────────
Pin 1       V-LED   →   150Ω resistor → GPIO 2
                         (also: 220µF cap between V-LED and GND)
Pin 2       LED GND →   GND
Pin 3       LED VCC →   3.3V  (through 150Ω series resistor)
Pin 4       S-GND   →   GND
Pin 5       Vout    →   GPIO 34  (ADC1 — analog input)
Pin 6       VCC     →   5V or 3.3V (check your module, GP2Y runs on 5V)

⚠ IMPORTANT: GP2Y1010AU0F runs best on 5V. If using 3.3V, readings
  will be lower than expected. Add a 10kΩ voltage divider on Vout
  if connecting 5V output to 3.3V ESP32 ADC pin.

  5V Vout → 10kΩ → GPIO34 (ADC) → 20kΩ → GND
  (Divider scales 5V → 3.3V safely)
```

### Capacitive Soil Moisture Sensor v1.2 → ESP32
```
Sensor Pin  →   ESP32 Pin
─────────────────────────
VCC         →   3.3V
GND         →   GND
AOUT        →   GPIO 35  (ADC1 — analog input)

Note: Sensor outputs a voltage proportional to moisture.
      Higher voltage = drier (capacitive inverse relationship).
      Calibrate SOIL_DRY_RAW and SOIL_WET_RAW in config.h.

Calibration steps:
  1. Leave sensor in open air → record ADC value → set SOIL_DRY_RAW
  2. Submerge tip in water   → record ADC value → set SOIL_WET_RAW
  3. Typical: Dry ≈ 3200, Wet ≈ 1100
```

### Buzzer → ESP32
```
Buzzer Pin  →   ESP32 Pin
─────────────────────────
VCC / +     →   GPIO 5
GND / –     →   GND
```

### Status LED → ESP32
```
LED Pin     →   ESP32 Pin
─────────────────────────
Anode (+)   →   220Ω resistor → GPIO 18
Cathode (–) →   GND
```

---

## ASCII Wiring Diagram

```
                    ┌──────────────────────────────────────┐
                    │          ESP32 Dev Board             │
                    │                                      │
  DHT11             │  GPIO4 ──────── DATA                 │
  ┌──────┐          │  3.3V ──────── VCC                   │
  │ VCC ─┼──────────┤  GND  ──────── GND                   │
  │ DAT ─┼──────────┤                                      │
  │ GND ─┼──────────┤                                      │
  └──────┘          │                                      │
                    │  GPIO2 ──150Ω──┐  ← PM2.5 LED ctrl  │
  PM2.5 GP2Y        │                │                     │
  ┌──────────┐      │               220µF to GND           │
  │ V-LED ───┼──────┤  GPIO34 ─────── Vout (analog)        │
  │ LED-GND ─┼──GND─┤  3.3V  ─────── VCC                  │
  │ Vout ────┼──────┤  GND   ─────── GND                   │
  │ VCC ─────┼──────┤                                      │
  └──────────┘      │                                      │
                    │  GPIO35 ─────── AOUT ← Soil sensor   │
  Soil Sensor       │  3.3V  ─────── VCC                   │
  ┌──────────┐      │  GND   ─────── GND                   │
  │ VCC ─────┼──────┤                                      │
  │ GND ─────┼──────┤  GPIO5 ──────── Buzzer +             │
  │ AOUT ────┼──────┤  GPIO18 ─220Ω── LED Anode            │
  └──────────┘      │  GND   ─────── LED Cathode + Buz –   │
                    └──────────────────────────────────────┘
```

---

## Power Supply Notes

- ESP32 powered via Micro-USB (5V from PC or adapter)
- DHT11, Soil sensor, Buzzer, LED: all powered from ESP32's 3.3V pin
- PM2.5 GP2Y: ideally 5V (use VIN pin = USB 5V), Vout → voltage divider

---

## Sensor Placement in Compost Bin

```
    ┌─────────────────────────────┐
    │         Compost Bin         │
    │                             │
    │   ┌─────────────────────┐   │
    │   │   Organic Waste     │   │
    │   │                     │   │
    │   │  [Soil Sensor tip]  │   │  ← Insert 3–5 cm deep into compost
    │   │        |            │   │
    │   │  [DHT11]            │   │  ← Mount on sidewall, 10cm from top
    │   └─────────────────────┘   │
    │                             │
    │  [PM2.5 Sensor]             │  ← Mount on lid or top air space
    │                             │
    │  [ESP32 + Buzzer + LED]     │  ← Mount outside bin on enclosure
    └─────────────────────────────┘
```

---

## Common Issues & Fixes

| Issue | Likely Cause | Fix |
|-------|-------------|-----|
| DHT11 returns NaN | Poor contact or no pull-up | Check wiring; add 4.7kΩ pull-up |
| PM2.5 always 0 | LED timing or voltage issue | Verify GPIO2/34 pins; check 150Ω resistor |
| Soil always 0% or 100% | Wrong calibration values | Re-calibrate SOIL_DRY_RAW / SOIL_WET_RAW |
| Cloud won't connect | Wrong WiFi credentials | Update WIFI_SSID/PASSWORD in config.h |
| Cloud variables not updating | Wrong DEVICE_ID or SECRET_KEY | Copy exact values from Arduino IoT Cloud |
