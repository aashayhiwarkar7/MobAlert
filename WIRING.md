# MobAlert — Wiring & Connections Guide

Complete hardware connection reference for the MobAlert Crowd Crush Early Warning System.

---

## Before You Start

**What you need:**
- ESP32 Dev Board (30-pin or 38-pin)
- Full-size breadboard
- Dupont jumper wires (male-to-male and male-to-female)
- USB cable for ESP32 power and flashing
- USB power supply rated at **1A minimum** (phone charger or power bank)

**Power rails on breadboard:**
- Connect ESP32 **3.3V** pin → breadboard **3.3V positive rail**
- Connect ESP32 **5V (Vin)** pin → breadboard **5V positive rail**
- Connect ESP32 **GND** pin → breadboard **negative GND rail**
- All component GND pins connect to this **same GND rail**

---

## Component 1 — MPU6050 (Accelerometer)

**What it does:** Detects ground vibration from crowd footsteps via its 3-axis accelerometer. The gyroscope axes are read but not used — only the accelerometer magnitude matters here.

**On breadboard:** Yes — seats directly on breadboard.

| MPU6050 Pin | Connects to | Notes |
|---|---|---|
| VCC | 3.3V rail | Do NOT use 5V — will damage the chip |
| GND | GND rail | |
| SDA | ESP32 GPIO 21 | Shared I2C data line with LCD |
| SCL | ESP32 GPIO 22 | Shared I2C clock line with LCD |
| XDA | Not connected | Auxiliary I2C — not used |
| XCL | Not connected | Auxiliary I2C — not used |
| AD0 | Not connected | Leaves default I2C address as 0x68 |
| INT | Not connected | Interrupt not used in this project |

**⚠️ Notes:**
- MPU6050 and LCD share the same SDA/SCL lines (GPIO 21 and 22). Wire both in parallel to the same ESP32 pins — this is standard I2C bus behaviour. They have different addresses so there is no conflict.
- Place MPU6050 as close to the ground surface or the structure being monitored as possible for best vibration pickup.
- At ±4G range (set in code), sensitivity = 8192 LSB/G. This gives better vibration resolution than the default ±2G range.

---

## Component 2 — HC-SR04 (Ultrasonic Distance Sensor)

**What it does:** Measures distance to the nearest person or object — used to estimate crowd density and proximity.

**On breadboard:** Yes — seats directly on breadboard.

| HC-SR04 Pin | Connects to | Notes |
|---|---|---|
| VCC | 5V (Vin) rail | Requires 5V — unreliable at 3.3V |
| GND | GND rail | |
| TRIG | ESP32 GPIO 5 | Trigger pulse output (10µs pulse) |
| ECHO | ESP32 GPIO 18 | Echo pulse input |

**⚠️ Notes:**
- HC-SR04 must be powered at 5V. At 3.3V it works intermittently and gives incorrect distance readings.
- The ECHO pin outputs 5V logic — ESP32 GPIO 18 is 5V tolerant so direct connection is acceptable on most ESP32 dev boards. If you want to be safe, use a voltage divider (1kΩ + 2kΩ) on the ECHO line.
- Point the sensor horizontally toward the crowd entry point for best detection.

---

## Component 3 — DHT11 (Temperature Sensor)

**What it does:** Monitors ambient temperature. Dense crowds generate significant body heat — a rising temperature reading is a supporting crowd crush indicator.

**On breadboard:** Yes — seats directly on breadboard.

| DHT11 Pin | Connects to | Notes |
|---|---|---|
| VCC (pin 1) | 3.3V rail | Also works at 5V |
| DATA (pin 2) | ESP32 GPIO 4 | Single-wire data |
| NC (pin 3) | Not connected | |
| GND (pin 4) | GND rail | |

**⚠️ Notes:**
- If using a raw 4-pin DHT11 (not on a breakout board), add a **10kΩ pull-up resistor** between DATA and VCC.
- Most DHT11 breakout modules already have the pull-up resistor built in — no extra resistor needed.
- DHT11 has a 1–2 second response time. It will not detect rapid temperature spikes instantly.
- If `readTemperature()` returns NaN (read failure), the code automatically uses the last known valid reading — this prevents the risk score from being corrupted.

---

## Component 4 — I2C LCD 16×2

**What it does:** Displays live alert status, risk score, and sensor snapshot readings.

**Off breadboard:** Connects via 4 Dupont wires.

| LCD Pin | Connects to | Notes |
|---|---|---|
| VCC | 5V (Vin) rail | Needs 5V for backlight |
| GND | GND rail | |
| SDA | ESP32 GPIO 21 | Shared I2C bus with MPU6050 |
| SCL | ESP32 GPIO 22 | Shared I2C bus with MPU6050 |

**⚠️ Notes:**
- Default I2C address is **0x27** on most modules. If screen stays blank after boot, change to **0x3F** in the code:
```cpp
LiquidCrystal_I2C lcd(0x3F, 16, 2);
```
- To find your LCD's exact address, run an I2C scanner sketch (see I2C Bus section below).
- LCD needs 5V for the backlight. At 3.3V backlight will be very dim or off.

---

## Component 5 — HC-05 (Bluetooth Module)

**What it does:** Sends wireless alerts and live sensor data to the operator's phone via Bluetooth Serial.

**Off breadboard:** Sits beside the breadboard connected via 4 Dupont wires.

| HC-05 Pin | Connects to | Notes |
|---|---|---|
| VCC | 3.3V rail | ⚠️ 3.3V ONLY — see warning |
| GND | GND rail | |
| TXD | ESP32 GPIO 32 (RX2) | HC-05 transmits → ESP32 receives |
| RXD | ESP32 GPIO 33 (TX2) | ESP32 transmits → HC-05 receives |
| STATE | Not connected | |
| EN | Not connected | |

**⚠️ Critical Notes:**
- **Power HC-05 at 3.3V ONLY.** At 5V it overheats, produces garbage data, or fails permanently.
- At 3.3V, both TX and RX lines operate at 3.3V logic — directly compatible with ESP32 GPIO. No voltage divider needed.
- **Why GPIO 32/33?** GPIO 16 and 17 are connected to flash memory on some ESP32 boards and cause random resets when used for UART. GPIO 32/33 are safe on all ESP32 dev boards.
- Default baud rate: **9600**. Default pairing PIN: **1234**.
- To receive on phone: install **Serial Bluetooth Terminal** (Android), pair with HC-05, connect.

---

## Component 6 — Servo Motor

**What it does:** Physically rotates to close an entry gate when Critical alert triggers (90°), and returns to open when alert clears (0°).

**Off breadboard:** Connects via its own 3-wire connector.

| Servo Wire | Colour | Connects to | Notes |
|---|---|---|---|
| Power | Red | 5V (Vin) rail | ⚠️ Must be 5V — see warning |
| Ground | Brown / Black | GND rail | |
| Signal | Orange / Yellow | ESP32 GPIO 26 | PWM signal |

**⚠️ Critical Notes:**
- **Never power servo from ESP32's 3.3V pin.** Under load a servo draws 500mA–1A. The 3.3V regulator cannot supply this and the ESP32 will brownout and reset — exactly when you don't want it to.
- Power from **5V Vin** which passes through directly from USB supply.
- If servo twitches randomly, your USB supply may not provide enough current. Use a charger rated at **1A or higher**.
- `gate.write(0)` = open. `gate.write(90)` = closed. Adjust angles if your physical gate needs different positions.

---

## Component 7 — Active Buzzer

**What it does:** Produces audio alert tones. Silent when Safe, short beep when Warning, rapid siren when Critical.

**On breadboard:** Yes.

| Buzzer Pin | Connects to | Notes |
|---|---|---|
| + (longer leg) | ESP32 GPIO 25 | Driven HIGH/LOW by ESP32 |
| − (shorter leg) | GND rail | |

**⚠️ Notes:**
- Must be an **active** buzzer — has internal oscillator, produces tone when pin goes HIGH. A passive buzzer will not work with this code.
- Active buzzers are usually labelled or have a small PCB on top.
- Draws ~30mA — within safe ESP32 GPIO limits (max 40mA per pin).

---

## Component 8 — LEDs (Green, Yellow, Red)

**What they do:** Exactly one LED on at all times corresponding to Safe, Warning, and Critical states.

**On breadboard:** Yes.

| LED | GPIO | Resistor |
|---|---|---|
| Green | 27 | 220Ω |
| Yellow | 14 | 220Ω |
| Red | 12 | 220Ω |

**Connection for each LED:**
```
ESP32 GPIO → [220Ω resistor] → LED + (longer leg) → LED − (shorter leg) → GND rail
```

**⚠️ Notes:**
- **220Ω resistor is mandatory** on each LED. Without it the GPIO pin will be damaged over time.
- LED polarity matters — longer leg (+) toward resistor, shorter leg (−) toward GND.
- Adjust resistor value for brightness: 100Ω = brighter, 470Ω = dimmer.

---

## I2C Shared Bus — MPU6050 and LCD

Both MPU6050 and the I2C LCD connect to the same two wires:
- **SDA → GPIO 21**
- **SCL → GPIO 22**

This is normal — multiple devices on one I2C bus is standard. Each device has a unique address:

| Device | I2C Address |
|---|---|
| MPU6050 | 0x68 |
| I2C LCD | 0x27 or 0x3F |

**I2C Scanner** — run this to verify both devices are detected:
```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Serial.println("Scanning I2C bus...");
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at: 0x");
      Serial.println(i, HEX);
    }
  }
  Serial.println("Done.");
}
void loop() {}
```
You should see two addresses — one for MPU6050 (0x68) and one for LCD (0x27 or 0x3F).

---

## Power Summary

| Rail | Source | Devices |
|---|---|---|
| 3.3V | ESP32 3.3V pin | MPU6050, DHT11, HC-05 |
| 5V | ESP32 Vin (USB passthrough) | HC-SR04, LCD, Servo |
| GND | ESP32 GND | Everything |

Use a USB supply rated at **1A minimum**. Servo under load can draw up to 500mA alone.

---

## Pre-Flash Checklist

Go through this before powering up:

- [ ] MPU6050 on 3.3V — not 5V
- [ ] HC-SR04 on 5V (Vin) — not 3.3V
- [ ] HC-05 on 3.3V — not 5V
- [ ] Servo on 5V (Vin) — not 3.3V pin
- [ ] LCD on 5V (Vin)
- [ ] 220Ω resistor on every LED positive leg
- [ ] MPU6050 SDA/SCL on same GPIO 21/22 as LCD
- [ ] HC-05 on GPIO 32/33 — not 16/17
- [ ] All GND pins on same common rail
- [ ] No loose wires touching each other
- [ ] USB supply is 1A or higher

---

## Common Problems and Fixes

| Problem | Likely cause | Fix |
|---|---|---|
| LCD blank after boot | Wrong I2C address | Change `0x27` to `0x3F` in code |
| LCD dim, no backlight | Powered at 3.3V | Move LCD VCC to 5V Vin |
| MPU6050 not found | Wrong SDA/SCL or wrong voltage | Confirm GPIO 21/22 and 3.3V power |
| HC-SR04 always 200cm | Powered at 3.3V | Move HC-SR04 VCC to 5V Vin |
| ESP32 resets randomly | Servo drawing too much current | Move servo to 5V Vin, use 1A+ supply |
| HC-05 garbage on phone | HC-05 powered at 5V | Move HC-05 VCC to 3.3V |
| Score stuck at 0 | History buffer not in loop | Confirm `scoreHistory[historyIndex]` update exists in `loop()` |
| No Bluetooth data | Wrong GPIO for HC-05 | Confirm GPIO 32 (RX2) and 33 (TX2) |
| Servo not moving | Underpowered | Use 1A+ USB supply |

---

*For full project code, see `MobAlert.ino`. For project overview, see `README.md`.*
