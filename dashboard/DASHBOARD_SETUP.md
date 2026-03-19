# Arduino IoT Cloud Dashboard Setup Guide
**SmartCompost IoT — Step-by-step configuration**

---

## Step 1 — Create an Arduino IoT Cloud Account

1. Go to [cloud.arduino.cc](https://cloud.arduino.cc)
2. Sign in or create a free account
3. Free tier supports up to 2 Things and 5 variables

---

## Step 2 — Create a New Thing

1. Click **Things** in the left sidebar
2. Click **+ Create Thing**
3. Name it: `SmartCompost`

---

## Step 3 — Add Variables

Add the following variables exactly as shown. Variable names must match the code in `thingProperties.h`.

| Variable Name | Type | Permission | Policy |
|---------------|------|------------|--------|
| `temperature` | Temperature (°C) | Read Only | On Change |
| `humidity` | Relative Humidity (%) | Read Only | On Change |
| `moisture` | Float | Read Only | On Change |
| `pm25` | Float | Read Only | On Change |
| `healthScore` | Float | Read Only | On Change |
| `daysElapsed` | Int | Read Only | On Change |
| `daysRemaining` | Int | Read Only | On Change |
| `compostStatus` | String | Read Only | On Change |
| `alerts` | String | Read Only | On Change |
| `buzzerEnabled` | Boolean | Read/Write | On Change |

---

## Step 4 — Associate Your Device

1. In the Thing, click **Associate Device**
2. Click **Set up new device** → **Arduino / ESP32**
3. Select board: **ESP32 Dev Module**
4. Name your device: `SmartCompost_ESP32`
5. **Copy the Device ID and Secret Key** → paste into `config.h`

---

## Step 5 — Network Configuration

1. Still in the Thing setup, enter your **WiFi SSID and Password**
2. This automatically populates the connection handler in the sketch

---

## Step 6 — Download / Configure Sketch

1. Click **Sketch** tab → **Open full editor**
2. The editor opens with `SmartCompost.ino`, `thingProperties.h` auto-generated
3. Replace content of each file with the code from this repo's `firmware/SmartCompost/`
4. Ensure `DEVICE_ID` and `SECRET_KEY` in `config.h` match what you copied

---

## Step 7 — Install Libraries

In Arduino Web Editor or Arduino IDE, install:

```
ArduinoIoTCloud           ≥ 1.15.0    (Arduino official)
Arduino_ConnectionHandler  ≥ 0.8.0    (Arduino official)
DHT sensor library         ≥ 1.4.4    (Adafruit)
Adafruit Unified Sensor    ≥ 1.1.9    (Adafruit)
```

---

## Step 8 — Upload Sketch

1. Select board: **ESP32 Dev Module**
2. Select the correct COM port
3. Click Upload
4. Open Serial Monitor at 115200 baud to see sensor output

---

## Step 9 — Create Dashboard

1. Go to **Dashboards** → **+ Create Dashboard**
2. Name it: `SmartCompost Monitor`
3. Click **+ Add Widget** for each of the following:

### Recommended Widget Layout

```
┌─────────────────────────────────────────────────────────────┐
│                 SmartCompost Monitor                        │
├─────────────┬─────────────┬─────────────┬───────────────────┤
│  Gauge      │  Gauge      │  Gauge      │   Value Widget    │
│  temperature│  humidity   │  moisture   │   pm25            │
│  0–80°C     │  0–100%     │  0–100%     │   µg/m³           │
├─────────────┴─────────────┴─────────────┴───────────────────┤
│         Status Card                   Health Gauge          │
│         compostStatus                 healthScore           │
│         (full width text)             0–100                 │
├────────────────────────┬────────────────────────────────────┤
│  Value: daysElapsed    │   Value: daysRemaining             │
│  "Days Running"        │   "Days Until Ready"               │
├────────────────────────┴────────────────────────────────────┤
│  Message Widget: alerts (full width)                        │
├─────────────────────────────────────────────────────────────┤
│  Toggle: buzzerEnabled — "Buzzer On/Off"                    │
└─────────────────────────────────────────────────────────────┘
```

### Widget-by-widget settings:

| Widget | Variable | Min | Max | Notes |
|--------|----------|-----|-----|-------|
| Gauge | temperature | 0 | 80 | Show warning at 65 |
| Gauge | humidity | 0 | 100 | — |
| Gauge | moisture | 0 | 100 | — |
| Value | pm25 | — | — | Show unit: µg/m³ |
| Gauge | healthScore | 0 | 100 | Color: 0–40 red, 40–70 yellow, 70–100 green |
| Value | daysElapsed | — | — | Label: "Days Running" |
| Value | daysRemaining | — | — | Label: "Est. Days Remaining" |
| Status | compostStatus | — | — | Large text |
| Message | alerts | — | — | Full width |
| Toggle | buzzerEnabled | — | — | Label: "Buzzer Alert" |

---

## Step 10 — Share Dashboard (Optional)

1. Click the **Share** icon on the dashboard
2. Enable **Public Link** to share with your mentor
3. Copy the link for submission

---

## Troubleshooting Cloud Connection

| Problem | Solution |
|---------|----------|
| "ArduinoCloud not connecting" | Check WiFi credentials in config.h |
| Variables not appearing | Ensure variable names match exactly (case-sensitive) |
| Upload fails | Install CP2102 / CH340 driver for your ESP32 |
| Serial shows "Cloud connected" but no data | Check DEVICE_ID and SECRET_KEY |
| Free tier limit | Upgrade plan or delete unused Things |
