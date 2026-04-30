# MobAlert 🚨
### Crowd Crush Early Warning System

> *"Seconds of warning. Lives saved."*

A real-time IoT safety system that detects crowd crush risk before it becomes fatal — using multi-sensor fusion, automated escalating alerts, and a physical entry barrier.

Built **solo** and demonstrated at **RESONANCE 2K26, IEEE Computational Intelligence Society, VIT Pune — April 2026.**

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

```
Risk Score = (Vibration × 0.40) + (Distance × 0.40) + (Temperature × 0.20)
```

A two-band mapping ensures the score escalates meaningfully:
- **Normal range** → maps to 0–40
- **Danger range** → maps to 40–100

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

> See `WIRING.md` for detailed per-component notes and warnings.

---

## Software Setup

**1. Install Libraries** (Arduino Library Manager):
- `MPU6050` by Electronic Cats
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor` by Adafruit
- `ESP32Servo` by Kevin Harrington
- `LiquidCrystal I2C` by Frank de Brabander

**2. Board:** ESP32 Dev Module

**3. Update Wi-Fi credentials** in `MobAlert.ino`:
```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

**4. Flash and open Serial Monitor at 115200 baud:**
```
[OK] MPU6050 connected.
[WiFi] Connected. Dashboard: http://192.168.x.x
[MobAlert] System ready. Monitoring started.
Score | Vib(G) | Dist(cm) | Temp(C) | Level
```

**5. If LCD is blank**, change `0x27` to `0x3F` in the LCD init line.

**6. Tune thresholds** to match your environment:
```cpp
#define VIB_WARN    0.8
#define VIB_CRIT    2.0
#define DIST_WARN   80
#define DIST_CRIT   40
#define TEMP_WARN   33.0
#define TEMP_CRIT   38.0
```

---

## Dashboard

Open `http://<ESP32-IP>` in any browser on the same Wi-Fi network. No internet required.

Shows: live risk score, vibration, distance, temperature, and a 40-reading history graph with color-coded danger zones.

---

## Bluetooth Monitoring

Install **Serial Bluetooth Terminal** on Android. Pair with HC-05 (PIN: `1234`). Connect.

**Live data stream (every 2 seconds):**
```
Vib:0.04 Dist:185.0 Temp:27.3 Score:12 Level:0
```

**Critical alert:**
```
==== MOBALERT ====
Status : CRITICAL — EVACUATE | Score: 82
Vib: 2.14G  Dist: 35cm  Temp: 36.2C
==================
```

---

## Alert Behaviour

| Level | Score | LED | Buzzer | Servo | Bluetooth |
|---|---|---|---|---|---|
| Safe | 0–40 | 🟢 Green | Silent | Open (0°) | Stream only |
| Warning | 40–70 | 🟡 Yellow | Beep / 2s | Open (0°) | Stream only |
| Critical | 70–100 | 🔴 Red | Rapid siren | Closed (90°) | Alert + stream |

---

## Demo Guide

| Action | Simulates | Expected |
|---|---|---|
| Stamp feet near MPU6050 | Crowd footstep vibration | Vibration score rises |
| Hand 40–80cm from HC-SR04 | Crowd density | Distance score rises |
| Warm air near DHT11 | Body heat buildup | Temp score rises |
| All three together | Crowd crush conditions | Critical triggers |

---

## Limitations

- HC-SR04 is single-point — measures one direction only
- DHT11 has 1–2 second response time and ±2°C accuracy
- Single node covers ~5m radius — full venue needs mesh network

---

## Future Improvements

- LoRa mesh for multi-node km-range coverage
- TinyML anomaly detection on ESP32
- GSM module for SMS alerts without Wi-Fi
- Pressure floor sensors for true density mapping
- Replace DHT11 with DHT22 for faster response

---

## Project Structure

```
MobAlert/
├── MobAlert.ino    — main sketch, fully commented
├── README.md       — this file
└── WIRING.md       — detailed per-component wiring guide
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
[github.com/aashayhiwarkar7](https://github.com/aashayhiwarkar7)

---

## License

MIT License — free to use, modify, and deploy.
