# MobAlert — Wiring & Connections Guide

Complete hardware connection reference for the MobAlert Crowd Crush Early Warning System.

---

## Before You Start

**What you need:**
- ESP32 Dev Board (30-pin or 38-pin)
- Full-size breadboard
- Dupont jumper wires (male-to-male and male-to-female)
- USB cable for ESP32 power and flashing

**Power rails on breadboard:**
- Connect ESP32 **3.3V** pin → breadboard **3.3V positive rail**
- Connect ESP32 **5V (Vin)** pin → breadboard **5V positive rail**
- Connect ESP32 **GND** pin → breadboard **negative GND rail**
- All component GND pins connect to this **same GND rail**

---

## Component 1 — MPU6050 (Accelerometer)

**What it does:** Detects ground vibration from crowd footsteps via its 3-axis accelerometer.

**On breadboard:** Yes — seats directly on breadboard.

| MPU6050 Pin | Connects to | Notes |
|---|---|---|
| VCC | 3.3V rail | Do NOT use 5V — will damage the chip |
| GND | GND rail | |
| SDA | ESP32 GPIO 21 | Shared I2C data line with LCD |
| SCL | ESP32 GPIO 22 | Shared I2C clock line with LCD |
| XDA | Not connected | Not used |
| XCL | Not connected | Not used |
| AD0 | Not connected | Leaves default I2C address as 0x68 |
| INT | Not connected | Interrupt not used in this project |

**⚠️ Notes:**
- MPU6050 and LCD share the same SDA/SCL lines (GPIO 21 and 22). Wire both in parallel to the same ESP32 pins — this is normal I2C bus behaviour. They have different addresses so there is no conflict.
- Place MPU6050 as close to the ground surface as possible for best vibration pickup.
- For consistent readings, the MPU6050 should be firmly mounted. Loose placement can introduce false vibration readings.
- Keep I2C wires short to reduce noise and improve communication stability.

---

## Component 2 — HC-SR04 (Ultrasonic Distance Sensor)

**What it does:** Measures distance to the nearest person/object — used to estimate crowd density.

**On breadboard:** Yes — seats directly on breadboard.

| HC-SR04 Pin | Connects to | Notes |
|---|---|---|
| VCC | 5V (Vin) rail | Requires 5V — will not work reliably at 3.3V |
| GND | GND rail | |
| TRIG | ESP32 GPIO 5 | Trigger pulse output from ESP32 |
| ECHO | ESP32 GPIO 18 | Echo pulse input to ESP32 |

**⚠️ Notes:**
- HC-SR04 must be powered at 5V. At 3.3V the sensor works intermittently and gives incorrect readings.
- The ECHO pin outputs 5V logic, while ESP32 GPIO operates at 3.3V. To ensure safe operation, a voltage divider is recommended.

  Use the following resistor divider:
    -Connect ECHO → 1kΩ resistor → junction point
    - From junction point → ESP32 GPIO 18
    - From junction point → 2kΩ resistor → GND

  This steps down the 5V signal to approximately 3.3V, protecting the ESP32 input pin. While direct connection may work during testing, the voltage divider is recommended for long-term reliability. *ESP32 GPIO pins are not officially 5V tolerant*. The voltage divider ensures safe long-term operation.
  Note: Without the divider, the system may still function, but prolonged use can stress the ESP32 input pin.
- Point the sensor horizontally toward the crowd entry point for best results.
- Ultrasonic readings can be affected by surface angle and soft materials (like clothing), which may absorb sound waves and reduce accuracy.

---

## Component 3 — DHT11 (Temperature Sensor)

**What it does:** Monitors ambient temperature. Dense crowds generate body heat — rising temperature is a supporting crowd crush indicator.

**On breadboard:** Yes — seats directly on breadboard.

| DHT11 Pin | Connects to | Notes |
|---|---|---|
| VCC (pin 1) | 3.3V rail | Also works at 5V but 3.3V is fine |
| DATA (pin 2) | ESP32 GPIO 4 | Single-wire data protocol |
| Not used (pin 3) | Not connected | |
| GND (pin 4) | GND rail | |

**⚠️ Notes:**
- Some DHT11 modules come on a small breakout board with 3 pins (VCC, DATA, GND) and a built-in pull-up resistor. Use that version if available — no external resistor needed.
- If using the raw 4-pin DHT11 sensor, add a **10kΩ pull-up resistor** between DATA and VCC.
- DHT11 has a 1–2 second response time. It will not detect rapid temperature changes instantly.
- DHT11 has limited accuracy (±2°C) and slow response. It is used here as a supporting indicator, not a primary safety parameter.

---

## Component 4 — I2C LCD 16×2

**What it does:** Displays live risk score, alert status, and sensor readings.

**Off breadboard:** Connects via 4 Dupont wires to breadboard rails and ESP32 pins.

| LCD Pin | Connects to | Notes |
|---|---|---|
| VCC | 5V (Vin) rail | LCD backlight needs 5V |
| GND | GND rail | |
| SDA | ESP32 GPIO 21 | Shared I2C bus with MPU6050 |
| SCL | ESP32 GPIO 22 | Shared I2C bus with MPU6050 |

**⚠️ Notes:**
- The default I2C address is **0x27** on most modules. If the screen stays completely blank after flashing, change it to **0x3F** in the code.
- To find your LCD's address, run an I2C scanner sketch — it will print all detected I2C devices to Serial Monitor.
- LCD needs 5V for the backlight. At 3.3V the backlight may be very dim or off.
- SDA and SCL are shared with MPU6050 — both devices sit on the same two wires simultaneously.

---

## Component 5 — HC-05 (Bluetooth Module)

**What it does:** Sends wireless alerts and live sensor data to the operator's phone via Bluetooth Serial.

**Off breadboard:** Sits beside the breadboard connected via 4 Dupont wires.

| HC-05 Pin | Connects to | Notes |
|---|---|---|
| VCC | 3.3V rail | ⚠️ 3.3V ONLY — see warning below |
| GND | GND rail | |
| TXD | ESP32 GPIO 32 (RX2) | HC-05 transmits → ESP32 receives |
| RXD | ESP32 GPIO 33 (TX2) | ESP32 transmits → HC-05 receives |
| STATE | Not connected | Status pin — not used |
| EN | Not connected | Enable pin — not used |

**⚠️ Critical Notes:**
- **Power HC-05 at 3.3V ONLY.** At 5V the module will overheat, give garbage data, or fail permanently.
- At 3.3V, both the TXD and RXD lines operate at 3.3V logic — compatible directly with ESP32 GPIO. No voltage divider needed.
- **Why GPIO 32/33?** GPIO 16 and 17 are used internally on some ESP32 boards (connected to flash memory). Using them for HC-05 causes random resets. GPIO 32/33 are safe on all ESP32 dev boards.
- Default HC-05 baud rate is 9600. Default pairing PIN is **1234**.
- To receive alerts on your phone: install **Serial Bluetooth Terminal** (Android), pair with HC-05, connect, and you will see live data in the terminal.

---

## Component 6 — Servo Motor

**What it does:** Physically rotates to close an entry gate when Critical alert is triggered. Rotates back to open when alert clears.

**Off breadboard:** Connects via its own 3-wire JST connector.

| Servo Wire | Colour (standard) | Connects to | Notes |
|---|---|---|---|
| Power | Red | 5V (Vin) rail | ⚠️ Must be 5V — see warning below |
| Ground | Brown or Black | GND rail | |
| Signal | Orange or Yellow | ESP32 GPIO 26 | PWM control signal |

**⚠️ Critical Notes:**
- **Never power the servo from ESP32's 3.3V pin.** Under load a servo draws 500mA–1A. The ESP32's 3.3V regulator cannot supply this and will brownout, resetting the entire system at the worst possible moment — during a Critical alert.
- Power the servo from **5V Vin** which comes directly from the USB power supply through the ESP32's onboard regulator — it can supply sufficient current.
- If the servo twitches randomly, your power supply may not be supplying enough current. Use a phone charger rated at 1A or higher.
- `gate.write(0)` = 0° = gate open. `gate.write(90)` = 90° = gate closed. Adjust these values if your physical gate setup requires different angles.

---

## Component 7 — Active Buzzer

**What it does:** Produces audio alert tones. Different patterns for Warning (short beep) and Critical (rapid siren).

**On breadboard:** Yes.

| Buzzer Pin | Connects to | Notes |
|---|---|---|
| + (positive, longer leg) | ESP32 GPIO 25 | Driven HIGH/LOW by ESP32 |
| − (negative, shorter leg) | GND rail | |

**⚠️ Notes:**
- Use an **active** buzzer (has internal oscillator, produces tone when pin goes HIGH). A passive buzzer requires PWM frequency control and will not work with this code.
- Active buzzers are usually labelled or have a small PCB on top. If you apply 3.3V directly and hear a continuous tone, it is active.
- The buzzer draws ~30mA — within safe limits for an ESP32 GPIO pin (max 40mA per pin).

---

## Component 8 — LEDs (Green, Yellow, Red)

**What they do:** Visual status indicators. Exactly one LED is on at a time corresponding to Safe, Warning, and Critical states.

**On breadboard:** Yes.

| LED | GPIO Pin | Resistor | Notes |
|---|---|---|---|
| Green | GPIO 27 | 220Ω | Safe state |
| Yellow | GPIO 14 | 220Ω | Warning state |
| Red | GPIO 12 | 220Ω | Critical state |

**Connection for each LED:**
```
ESP32 GPIO → [220Ω resistor] → LED positive (longer leg) → LED negative (shorter leg) → GND rail
```

**⚠️ Notes:**
- **220Ω resistor is mandatory** on each LED. Without it, the LED draws too much current and will permanently damage the ESP32 GPIO pin over time.
- LED polarity matters — longer leg (+) toward resistor, shorter leg (−) toward GND.
- If a 220Ω resistor makes the LED too dim, use 100Ω. If you want dimmer LEDs, use 330Ω or 470Ω.

---

## I2C Bus — Shared Between MPU6050 and LCD

Both MPU6050 and the I2C LCD share the same two wires:
- **SDA → GPIO 21**
- **SCL → GPIO 22**

This is standard I2C bus behaviour. Multiple devices on the same bus is normal and expected. Each device has a unique address so they never interfere:

| Device | I2C Address |
|---|---|
| MPU6050 | 0x68 (default, AD0 low) |
| I2C LCD | 0x27 or 0x3F (depends on module) |

To verify both are detected, run this I2C scanner in a separate sketch:
```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at: 0x");
      Serial.println(i, HEX);
    }
  }
}
void loop() {}
```
You should see two addresses printed — one for MPU6050 and one for the LCD.

---

## Power Summary

| Rail | Source | Powers |
|---|---|---|
| 3.3V | ESP32 3.3V pin | MPU6050, DHT11, HC-05 |
| 5V | ESP32 Vin pin (USB passthrough) | HC-SR04, LCD, Servo |
| GND | ESP32 GND pin | Everything |

**USB power supply:** Use a phone charger or power bank rated at **1A minimum**. The servo alone can draw up to 500mA under load. A laptop USB port may not supply enough current and can cause instability. All devices must share a common ground. Failure to do so will result in unstable or non-functional behavior.

---

## Quick Sanity Check Before Flashing

Go through this checklist before powering up:

- [ ] MPU6050 on 3.3V — not 5V
- [ ] HC-SR04 on 5V (Vin) — not 3.3V
- [ ] HC-05 on 3.3V — not 5V
- [ ] Servo on 5V (Vin) — not 3.3V pin
- [ ] LCD on 5V (Vin)
- [ ] 220Ω resistor on every LED positive leg
- [ ] MPU6050 SDA/SCL wired to same GPIO 21/22 as LCD
- [ ] HC-05 on GPIO 32/33 — not 16/17
- [ ] All GND pins connected to same common rail
- [ ] No loose wires touching each other

---

## Common Problems and Fixes

| Problem | Likely cause | Fix |
|---|---|---|
| LCD blank after boot | Wrong I2C address | Change `0x27` to `0x3F` in code |
| LCD dim, no backlight | Powered at 3.3V | Move LCD VCC to 5V Vin |
| MPU6050 not found in Serial Monitor | Wrong SDA/SCL or wrong voltage | Check GPIO 21/22, confirm 3.3V power |
| HC-SR04 always returns 200cm | Powered at 3.3V | Move HC-SR04 VCC to 5V Vin |
| ESP32 resets randomly | Servo pulling too much current | Move servo to 5V Vin, use 1A+ charger |
| HC-05 garbage data on phone | HC-05 powered at 5V | Move HC-05 VCC to 3.3V |
| Risk score never rises above 0 | History buffer not updating | Confirm `scoreHistory[historyIndex]` update is in loop() |
| No Bluetooth data on phone | Wrong GPIO for HC-05 | Confirm GPIO 32 (RX2) and 33 (TX2) in code |

---

*For full project code and README, see the main repository files.*
