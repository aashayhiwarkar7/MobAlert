# MobAlert 🚨
### Crowd Crush Early Warning System

> *"Seconds of warning. Lives saved."*

A real-time IoT safety system that detects crowd crush risk before it becomes fatal — using multi-sensor fusion, automated escalating alerts, and a physical entry barrier.

Built **solo** and demonstrated at **RESONANCE 2K26, IEEE VIT Pune Hackathon, April 2026.**

---

## The Problem

Crowd crushes kill hundreds of people every year. They build over minutes but turn fatal in seconds. No affordable, ground-level early warning system exists for event organizers.

**Direct reference:** Chinnaswamy Stadium, Bengaluru — RCB IPL win celebration, May 2025. A sudden crowd surge caused multiple casualties outside the stadium. Zero automated warning systems were in place.

| Solution | Cost | Works offline | Auto response | Privacy safe |
|---|---|---|---|---|
| Existing crowd systems | ₹50,000+ | ❌ | ❌ | ✅ |
| Camera-based AI | ₹20,000+ | ❌ | ❌ | ❌ |
| **MobAlert** | **< ₹1,000** | **✅** | **✅** | **✅** |

---

## How It Works

MobAlert reads 3 sensors continuously and fuses their data into a single **Risk Score (0–100)**. When thresholds are crossed, it triggers escalating responses — culminating in a physical servo gate that blocks new entries.

```
MPU6050  →  Floor vibration intensity (crowd footsteps)
HC-SR04  →  Distance to nearest person (crowd density)
DHT11    →  Ambient temperature (body heat buildup)
              ↓
    ESP32 — weighted sensor fusion
              ↓
        Risk Score 0–100
              ↓
   ┌──────────────────────────────────────┐
   │  0–40   SAFE      Green LED only     │
   │  40–70  WARNING   Yellow LED         │
   │                   Intermittent beep  │
   │  70–100 CRITICAL  Red LED            │
   │                   Continuous siren   │
   │                   Servo closes gate  │
   │                   Bluetooth alert    │
   └──────────────────────────────────────┘
              ↓
   Wi-Fi dashboard + Serial Monitor
```

### Risk Score Formula

Each sensor reading is mapped to a 0–100 sub-score, then combined with weights:

```
Risk Score = (Vibration × 0.40) + (Distance × 0.40) + (Temperature × 0.20)
```

A two-band mapping system ensures the score escalates meaningfully:
- **Normal range** → maps to 0–40 (low risk)
- **Danger range** → maps to 40–100 (escalating risk)

This means a single sensor at its warning threshold alone will push the score to ~40, which correctly triggers Warning level.

---

## Hardware

| Component | Role | Cost (approx) |
|---|---|---|
| ESP32 Dev Board | Controller + Wi-Fi + Bluetooth | ₹400–500 |
| MPU6050 | Crowd footstep vibration via accelerometer | ₹80–120 |
| HC-SR04 | Crowd proximity / density estimation | ₹40–60 |
| DHT11 | Ambient heat buildup from body heat | ₹50–80 |
| HC-05 | Bluetooth alert to operator's phone | ₹100–150 |
| Servo Motor | Physical entry barrier gate | ₹80–120 |
| I2C LCD 16×2 | Live status and readings display | ₹150–200 |
| LEDs (G/Y/R) | Visual alert status indicators | ₹10 |
| 220Ω Resistors | LED current limiting (one per LED) | ₹5 |
| Breadboard + wires | Connections | ₹80–100 |

**Total: < ₹1,000**

---

## Wiring

### Full Pin Map

| Component | Pin | ESP32 GPIO |
|---|---|---|
| MPU6050 | VCC | 3.3V |
| MPU6050 | GND | GND |
| MPU6050 | SDA | 21 |
| MPU6050 | SCL | 22 |
| HC-SR04 | VCC | 5V (Vin) |
| HC-SR04 | GND | GND |
| HC-SR04 | TRIG | 5 |
| HC-SR04 | ECHO | 18 |
| DHT11 | VCC | 3.3V |
| DHT11 | GND | GND |
| DHT11 | DATA | 4 |
| I2C LCD | VCC | 5V (Vin) |
| I2C LCD | GND | GND |
| I2C LCD | SDA | 21 |
| I2C LCD | SCL | 22 |
| HC-05 | VCC | 3.3V |
| HC-05 | GND | GND |
| HC-05 | TX | 32 (RX2) |
| HC-05 | RX | 33 (TX2) |
| Servo | VCC (red) | 5V (Vin) |
| Servo | GND (brown) | GND |
| Servo | Signal (orange) | 26 |
| Buzzer | + | 25 |
| Buzzer | − | GND |
| LED Green | + via 220Ω | 27 |
| LED Yellow | + via 220Ω | 14 |
| LED Red | + via 220Ω | 12 |
| All GNDs | − | Common GND rail |

### ⚠️ Critical Wiring Notes

**I2C shared bus:** MPU6050 and LCD both connect to GPIO 21 (SDA) and GPIO 22 (SCL). This is intentional — they have different I2C addresses so there is no conflict. Wire both in parallel to the same pins.

**HC-05 voltage:** Power HC-05 from **3.3V only**. At 5V the module will overheat and fail. At 3.3V the GPIO 33 TX line is safe to connect directly — no voltage divider needed.

**Servo power:** The servo must be powered from the **5V Vin pin** directly. Never power it from the 3.3V pin — under load the servo draws too much current and will reset the ESP32 mid-demo.

**LED resistors:** Each LED must have a **220Ω resistor** on its positive leg. Without it the GPIO pin will be damaged over time.

**GPIO 16/17:** These pins are used internally on some ESP32 boards. HC-05 is wired to GPIO 32/33 to avoid conflicts.

---

## Software Setup

### Step 1 — Install Libraries
Open Arduino IDE → Tools → Manage Libraries, search and install:
- `MPU6050` by Electronic Cats
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor` by Adafruit
- `ESP32Servo` by Kevin Harrington
- `LiquidCrystal I2C` by Frank de Brabander

### Step 2 — Board Configuration
- Board: **ESP32 Dev Module**
- Upload Speed: 115200
- Port: your ESP32's COM port

### Step 3 — Update Wi-Fi Credentials
In `MobAlert.ino`, find and update:
```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```
If Wi-Fi is unavailable, the system runs in offline mode — all hardware alerts (LEDs, buzzer, servo, Bluetooth) still work normally.

### Step 4 — Flash and Monitor
- Upload the sketch to ESP32
- Open Serial Monitor at **115200 baud**
- Expected output on boot:
```
[OK] MPU6050 connected.
[WiFi] Connected. Dashboard: http://192.168.x.x
[MobAlert] System ready. Monitoring started.
Score | Vib(G) | Dist(cm) | Temp(C) | Level
------+--------+----------+---------+-------
   12 |  0.04  |    185    |   27.3  |   0
```
- Open the IP address in any browser on the same network for the live dashboard

### Step 5 — Tune Thresholds
Before deploying or demoing, adjust these values to match your environment:
```cpp
#define VIB_WARN    0.8     // Lower = more sensitive to vibration
#define VIB_CRIT    2.0     // Increase if false Critical triggers occur
#define DIST_WARN   80      // cm — increase for open spaces
#define DIST_CRIT   40      // cm — distance at which crowd is dangerously close
#define TEMP_WARN   33.0    // Lower in hot environments
#define TEMP_CRIT   38.0
```

### Step 6 — LCD Address
If the LCD stays blank after boot, change the I2C address:
```cpp
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Try 0x3F instead of 0x27
```

---

## Dashboard

ESP32 hosts a live dashboard — **no internet required**. Works on any phone or laptop connected to the same Wi-Fi network.

Open `http://<ESP32-IP>` in any browser.

**Dashboard shows:**
- Color-coded status bar (Safe / Warning / Critical with pulsing animation)
- Live cards: Risk Score, Vibration (G), Distance (cm), Temperature (°C)
- Risk score history graph — last 40 readings with colored danger zones

---

## Bluetooth Monitoring

**Setup:**
1. Install **Serial Bluetooth Terminal** (Android) or any Bluetooth Serial app
2. Pair with HC-05 — default PIN is `1234`
3. Connect to HC-05 in the app

**What you receive:**

Continuous live data every 250ms:
```
Vib:0.04 Dist:185.0 Temp:27.3 Score:12 Level:0
```

Critical alert (on entering Critical, then every 10 seconds):
```
==== MobAlert ALERT ====
Status : CRITICAL — EVACUATE | Score: 82
Vib: 2.14G  Dist: 35cm  Temp: 36.2C
==========================
```

---

## Alert Behaviour Summary

| Level | Score | Green LED | Yellow LED | Red LED | Buzzer | Servo | Bluetooth |
|---|---|---|---|---|---|---|---|
| Safe | 0–40 | ON | off | off | Silent | Open (0°) | Stream only |
| Warning | 40–70 | off | ON | off | Beep / 2s | Open (0°) | Stream only |
| Critical | 70–100 | off | off | ON | Rapid siren | Closed (90°) | Alert + stream |

---

## Demo Guide

To simulate crowd conditions during testing or presentation:

| Action | What it simulates | Expected result |
|---|---|---|
| Stamp feet / tap table near MPU6050 | Crowd footstep vibration | Vibration score rises |
| Hold hand 40–80cm from HC-SR04 | Crowd density | Distance score rises |
| Breathe warm air near DHT11 | Body heat buildup | Temperature score rises |
| All three simultaneously | Crowd crush conditions | Critical triggers |

**What judges see at Critical:**
1. Red LED activates
2. Buzzer rapid siren starts
3. Servo arm physically rotates to 90° — gate closes
4. Phone receives "CRITICAL — EVACUATE" alert

---

## Limitations & Honest Assessment

- **HC-SR04** is a single-point sensor — measures one direction only. A real deployment would need pressure mats or multiple nodes.
- **DHT11** has a 1–2 second response time and ±2°C accuracy — fast heat spikes may not register immediately. DHT22 would improve this.
- **Single node** — covers approximately a 5-metre radius. Full venue coverage needs a mesh network.

---

## Future Improvements

- LoRa mesh network — multi-node, km-range, no Wi-Fi dependency
- TinyML anomaly detection running on ESP32 directly
- GSM module — SMS alerts without Wi-Fi or internet
- Pressure-sensitive floor mats — true crowd density mapping
- Mobile app for event control room monitoring
- Replace DHT11 with DHT22 for faster, more accurate temperature readings

---

## Project Structure

```
MobAlert/
├── MobAlert.ino    — main sketch with full comments
└── README.md         — this file
```

---

## Built At

**RESONANCE 2K26**
IEEE Computational Intelligence Society
Department of CSE (AI & ML), VIT Pune Bibwewadi
April 17, 2026 — IoT Track, Sponsored by DesignBytes

---

## Author

**Aashay Hiwarkar**
Mechanical Engineering → Embedded Systems
[github.com/aashayhiwarkar7]
[https://www.linkedin.com/in/aashay-hiwarkar-7a0180376/]

---

## License

MIT License — free to use, modify, and deploy.
