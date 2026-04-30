// ╔══════════════════════════════════════════════════════════════╗
// ║              MobAlert — Crowd Crush Early Warning System     ║
// ║                                                              ║
// ║  A real-time IoT safety system that detects crowd crush      ║
// ║  risk using multi-sensor fusion. Triggers escalating         ║
// ║  alerts and physically blocks entry at critical levels.      ║
// ║                                                              ║
// ║  Built at RESONANCE 2K26                                     ║
// ║  IEEE Computational Intelligence Society, VIT Pune           ║
// ║  April 2026                                                  ║
// ║  Author: Aashay Hiwarkar                                     ║
// ╚══════════════════════════════════════════════════════════════╝
//
//  HARDWARE REQUIRED:
//    ESP32 Dev Board     — main controller + Wi-Fi
//    MPU6050             — detects crowd footstep vibration
//    HC-SR04             — measures crowd proximity / density
//    DHT11               — monitors ambient heat buildup
//    HC-05               — sends Bluetooth alert to operator
//    I2C LCD 16x2        — displays live status and readings
//    Servo Motor         — physical entry barrier gate
//    LEDs (G / Y / R)    — visual alert status
//    Active Buzzer       — audio alert
//
//  WIRING SUMMARY:
//    MPU6050  → SDA:21  SCL:22  VCC:3.3V
//    HC-SR04  → TRIG:5  ECHO:18 VCC:5V(Vin)
//    DHT11    → DATA:4  VCC:3.3V
//    LCD      → SDA:21  SCL:22  VCC:5V(Vin)   [shared I2C bus with MPU6050]
//    HC-05    → TX:32   RX:33   VCC:3.3V       [NOT 5V — will damage module]
//    Servo    → SIG:26  VCC:5V(Vin)            [NOT 3.3V — will reset ESP32]
//    Buzzer   → +:25
//    LED G    → +:27 via 220Ω
//    LED Y    → +:14 via 220Ω
//    LED R    → +:12 via 220Ω
//
//  LIBRARIES (install via Arduino Library Manager):
//    MPU6050                by Electronic Cats
//    DHT sensor library     by Adafruit
//    Adafruit Unified Sensor by Adafruit
//    ESP32Servo             by Kevin Harrington
//    LiquidCrystal I2C      by Frank de Brabander
// ═══════════════════════════════════════════════════════════════


// ── INCLUDES ──────────────────────────────────────────────────
#include <Wire.h>               // I2C bus — used by MPU6050 and LCD
#include <MPU6050.h>            // Accelerometer + gyroscope driver
#include <DHT.h>                // DHT11/DHT22 temperature sensor driver
#include <ESP32Servo.h>         // Servo control (ESP32-safe version)
#include <LiquidCrystal_I2C.h> // I2C LCD driver
#include <WiFi.h>               // ESP32 Wi-Fi stack
#include <WebServer.h>          // Simple HTTP server for dashboard


// ── PIN DEFINITIONS ───────────────────────────────────────────
#define TRIG_PIN    5     // HC-SR04: trigger output
#define ECHO_PIN    18    // HC-SR04: echo input
#define DHT_PIN     4     // DHT11: data pin
#define BUZZER_PIN  25    // Active buzzer positive pin
#define SERVO_PIN   26    // Servo signal pin
#define LED_GREEN   27    // Green LED  — Safe state
#define LED_YELLOW  14    // Yellow LED — Warning state
#define LED_RED     12    // Red LED    — Critical state
#define HC05_RX     32    // UART2 RX ← HC-05 TX  (GPIO 16/17 occupied on some boards)
#define HC05_TX     33    // UART2 TX → HC-05 RX


// ── SENSOR THRESHOLDS ─────────────────────────────────────────
// These define the Warning and Critical trigger points per sensor.
// Tune these values in Serial Monitor during testing.
// Lower VIB values = more sensitive. Lower DIST values = triggers only when very close.

#define VIB_WARN    0.8     // G-force: footstep vibration warning
#define VIB_CRIT    2.0     // G-force: heavy crowd vibration critical
#define DIST_WARN   80      // cm: crowd nearby — warning
#define DIST_CRIT   40      // cm: crowd very close — critical
#define TEMP_WARN   33.0    // °C: body heat buildup — warning
#define TEMP_CRIT   38.0    // °C: heavy heat buildup — critical


// ── RISK SCORE WEIGHTS ────────────────────────────────────────
// Each sensor contributes a weighted portion to the final 0–100 score.
// Vibration and distance are the most direct crowd indicators (40% each).
// Temperature is a slower, supporting signal (20%).
// All three weights must add up to exactly 1.0.

#define W_VIB   0.40
#define W_DIST  0.40
#define W_TEMP  0.20


// ── Wi-Fi CREDENTIALS ─────────────────────────────────────────
// Update before flashing. If Wi-Fi fails, system runs offline —
// all hardware alerts still work, only the dashboard is unavailable.

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";


// ── OBJECT DECLARATIONS ───────────────────────────────────────
MPU6050           mpu;                   // MPU6050 sensor object
DHT               dht(DHT_PIN, DHT11);  // DHT11 object (change DHT11→DHT22 if upgrading)
Servo             gate;                  // Servo barrier object
LiquidCrystal_I2C lcd(0x27, 16, 2);    // LCD at address 0x27 — try 0x3F if screen stays blank
WebServer         server(80);            // Web server on port 80


// ── GLOBAL STATE ──────────────────────────────────────────────
int   alertLevel = 0;       // 0 = Safe | 1 = Warning | 2 = Critical
float riskScore  = 0.0;     // Current weighted risk score (0–100)
float lastVib    = 0.0;     // Last valid vibration reading in G-force
float lastDist   = 200.0;   // Last valid distance reading in cm
float lastTemp   = 25.0;    // Last valid temperature in °C

// Circular history buffer — stores last 40 risk scores for dashboard graph
#define HISTORY_SIZE 40
float scoreHistory[HISTORY_SIZE];
int   historyIndex = 0;

// Timing variables for non-blocking operations
unsigned long lastBuzzerToggle = 0;   // Tracks buzzer pattern timing
bool          buzzerState      = false;
unsigned long lastLCDUpdate    = 0;   // Prevents LCD flickering
unsigned long lastAlertSent    = 0;   // Prevents Bluetooth spam
unsigned long lastDataSent     = 0;   // Throttles continuous BT data stream


// ╔══════════════════════════════════════════════════════════════╗
// ║  SETUP — runs once on boot                                   ║
// ╚══════════════════════════════════════════════════════════════╝
void setup() {

  // Serial Monitor at 115200 baud — for debug output on your laptop via USB
  Serial.begin(115200);

  // UART2 at 9600 baud — for HC-05 Bluetooth module (HC-05 factory default is 9600)
  // These are two independent hardware UARTs — different speeds do not conflict
  Serial2.begin(9600, SERIAL_8N1, HC05_RX, HC05_TX);

  // Configure GPIO directions
  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is silent on boot

  // Initialise MPU6050
  // ±4G range chosen over default ±2G for better vibration sensitivity
  // without losing resolution on normal movement
  Wire.begin(21, 22);
  mpu.initialize();
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);
  if (mpu.testConnection()) {
    Serial.println("[OK] MPU6050 connected.");
  } else {
    Serial.println("[ERR] MPU6050 not found — check SDA/SCL wiring and power.");
  }

  // Initialise DHT11
  dht.begin();

  // Servo starts at 0° = gate open
  gate.attach(SERVO_PIN);
  gate.write(0);
  delay(300); // Give servo time to reach position before loop starts

  // LCD boot message
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("MobAlert v1     ");
  lcd.setCursor(0, 1); lcd.print("Booting...      ");
  delay(1500);
  lcd.clear();

  // Attempt Wi-Fi connection (max ~10 seconds)
  // System continues in offline mode if connection fails
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected. Dashboard: http://");
    Serial.println(WiFi.localIP());
    lcd.setCursor(0, 0); lcd.print("WiFi OK         ");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Failed — offline mode. Hardware alerts still active.");
    lcd.setCursor(0, 0); lcd.print("WiFi FAILED     ");
    lcd.setCursor(0, 1); lcd.print("Offline mode    ");
  }
  delay(2000);
  lcd.clear();

  // Register dashboard URL handlers
  server.on("/",     handleRoot); // Serves HTML dashboard page
  server.on("/data", handleData); // Serves live JSON data to dashboard
  server.begin();

  // Zero-fill history buffer so graph starts clean
  memset(scoreHistory, 0, sizeof(scoreHistory));

  // Print Serial Monitor header
  Serial.println("[MobAlert] System ready. Monitoring started.");
  Serial.println("Score | Vib(G) | Dist(cm) | Temp(C) | Level");
  Serial.println("------+--------+----------+---------+-------");
}


// ╔══════════════════════════════════════════════════════════════╗
// ║  MAIN LOOP — runs continuously (~4 times per second)         ║
// ╚══════════════════════════════════════════════════════════════╝
void loop() {
  unsigned long now = millis();

  // ── 1. READ ALL SENSORS ───────────────────────────────────
  lastVib  = readVibration();   // MPU6050 accelerometer magnitude
  lastDist = readUltrasonic();  // HC-SR04 distance measurement
  lastTemp = readTemperature(); // DHT11 temperature

  // ── 2. CALCULATE RISK SCORE ───────────────────────────────
  // Weighted fusion of all 3 sensor scores → single 0–100 value
  riskScore = calculateRisk(lastVib, lastDist, lastTemp);

  // Store in circular buffer for dashboard graph
  scoreHistory[historyIndex] = riskScore;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;

  // ── 3. DETERMINE ALERT LEVEL ──────────────────────────────
  int prevLevel = alertLevel;   // Store previous level to detect transitions
  if      (riskScore >= 70) alertLevel = 2; // Critical
  else if (riskScore >= 40) alertLevel = 1; // Warning
  else                      alertLevel = 0; // Safe

  // ── 4. UPDATE PHYSICAL OUTPUTS ────────────────────────────
  updateLEDs();           // One LED on based on alert level
  updateBuzzer(now);      // Pattern changes per level (silent/beep/siren)
  updateServo();          // Gate: open at 0°, closed at 90° on Critical

  // LCD updates every 500ms to prevent constant flicker
  if (now - lastLCDUpdate > 500) {
    updateLCD();
    lastLCDUpdate = now;
  }

  // ── 5. BLUETOOTH ALERT ────────────────────────────────────
  // Sends a formatted alert message to HC-05 when Critical is first reached
  // Re-sends every 10 seconds while Critical level continues
  // This prevents flooding the operator's phone with repeated messages
  if (alertLevel == 2) {
    if (prevLevel < 2 || (now - lastAlertSent > 10000)) {
      sendBluetoothAlert();
      lastAlertSent = now;
    }
  }

  // ── 6. CONTINUOUS BLUETOOTH DATA STREAM ───────────────────
  // Sends live readings every 2 seconds to the operator's phone
  // Throttled to prevent terminal scroll flood
  if (now - lastDataSent > 2000) {
    Serial2.print("Vib:");    Serial2.print(lastVib, 2);
    Serial2.print(" Dist:");  Serial2.print(lastDist);
    Serial2.print(" Temp:");  Serial2.print(lastTemp);
    Serial2.print(" Score:"); Serial2.print((int)riskScore);
    Serial2.print(" Level:"); Serial2.println(alertLevel);
    lastDataSent = now;
  }

  // ── 7. SERIAL MONITOR OUTPUT ──────────────────────────────
  Serial.printf("  %3d  |  %.2f  |    %.0f     |   %.1f   |   %d\n",
    (int)riskScore, lastVib, lastDist, lastTemp, alertLevel);

  // ── 8. SERVE WEB DASHBOARD REQUESTS ──────────────────────
  server.handleClient();

  delay(250); // 250ms loop = ~4 readings per second
}


// ╔══════════════════════════════════════════════════════════════╗
// ║  SENSOR FUNCTIONS                                            ║
// ╚══════════════════════════════════════════════════════════════╝

// readVibration()
// ──────────────
// Gets raw 6-axis data from MPU6050 (3 accel + 3 gyro axes)
// Only accelerometer axes are used — gyroscope is ignored here
// At ±4G range: 1G = 8192 raw units (LSB/G)
// We calculate the vector magnitude and subtract 1G static gravity
// Result = net dynamic acceleration = vibration intensity in G-force
float readVibration() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float axG = ax / 8192.0; // X-axis in G
  float ayG = ay / 8192.0; // Y-axis in G
  float azG = az / 8192.0; // Z-axis in G

  // 3D vector magnitude − 1G baseline = pure vibration
  return abs(sqrt(axG*axG + ayG*ayG + azG*azG) - 1.0);
}

// readUltrasonic()
// ────────────────
// Sends a 10 microsecond pulse on TRIG pin
// Measures how long the echo takes to return on ECHO pin
// Distance = (duration × speed of sound) / 2
// Returns 200cm if no echo received (timeout) = clear path
float readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 25ms timeout → max distance ~425cm
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return 200.0;
  return constrain(duration * 0.034 / 2.0, 2.0, 200.0);
}

// readTemperature()
// ─────────────────
// Reads temperature from DHT11
// DHT11 can occasionally fail a read and return NaN
// In that case we return the last known valid temperature
// to avoid NaN values corrupting the risk score
float readTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) return lastTemp;
  return t;
}


// ╔══════════════════════════════════════════════════════════════╗
// ║  RISK SCORE CALCULATION                                      ║
// ╚══════════════════════════════════════════════════════════════╝

// mapF()
// ──────
// Float version of Arduino's built-in map() function
// Also includes constrain so the output never goes out of bounds
// Used to map each sensor reading to a 0–100 score
float mapF(float x, float iMin, float iMax, float oMin, float oMax) {
  float mapped = (x - iMin) * (oMax - oMin) / (iMax - iMin) + oMin;
  return constrain(mapped, min(oMin, oMax), max(oMin, oMax));
}

// calculateRisk()
// ───────────────
// Maps each sensor to a 0–100 sub-score using a two-band system:
//   Normal range  → maps to 0–40  (low risk zone)
//   Danger range  → maps to 40–100 (escalating risk zone)
// Then combines sub-scores using weighted sum → final 0–100 score
//
// Why two bands?
//   A reading at the very bottom of the danger zone (e.g. VIB_WARN)
//   should score 40, not 0. This ensures Warning triggers reliably
//   even when only one sensor is elevated.
float calculateRisk(float vib, float dist, float temp) {

  // Vibration sub-score
  float vibScore;
  if      (vib >= VIB_CRIT) vibScore = 100;
  else if (vib >= VIB_WARN) vibScore = mapF(vib, VIB_WARN, VIB_CRIT, 40, 100);
  else                      vibScore = mapF(vib, 0, VIB_WARN, 0, 40);

  // Distance sub-score — closer = higher score (inverted mapping)
  float distScore;
  if      (dist <= DIST_CRIT) distScore = 100;
  else if (dist <= DIST_WARN) distScore = mapF(dist, DIST_CRIT, DIST_WARN, 100, 40);
  else                        distScore = mapF(dist, DIST_WARN, 200, 40, 0);

  // Temperature sub-score
  float tempScore;
  if      (temp >= TEMP_CRIT) tempScore = 100;
  else if (temp >= TEMP_WARN) tempScore = mapF(temp, TEMP_WARN, TEMP_CRIT, 40, 100);
  else                        tempScore = mapF(temp, 20, TEMP_WARN, 0, 40);

  // Weighted combination → final score
  return constrain(
    (vibScore * W_VIB) + (distScore * W_DIST) + (tempScore * W_TEMP),
    0, 100
  );
}


// ╔══════════════════════════════════════════════════════════════╗
// ║  OUTPUT FUNCTIONS                                            ║
// ╚══════════════════════════════════════════════════════════════╝

// updateLEDs()
// ────────────
// Exactly one LED on at all times — reflects current alert level
// Green = Safe | Yellow = Warning | Red = Critical
void updateLEDs() {
  digitalWrite(LED_GREEN,  alertLevel == 0);
  digitalWrite(LED_YELLOW, alertLevel == 1);
  digitalWrite(LED_RED,    alertLevel == 2);
}

// updateBuzzer()
// ──────────────
// Non-blocking buzzer control using millis() — no delay() calls
// Safe     → Completely silent
// Warning  → Short single beep every 2 seconds (200ms on, 1800ms off)
// Critical → Rapid siren toggling every 150ms
void updateBuzzer(unsigned long now) {
  if (alertLevel == 0) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;

  } else if (alertLevel == 1) {
    if (!buzzerState && now - lastBuzzerToggle > 2000) {
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerState = true;
      lastBuzzerToggle = now;
    } else if (buzzerState && now - lastBuzzerToggle > 200) {
      digitalWrite(BUZZER_PIN, LOW);
      buzzerState = false;
      lastBuzzerToggle = now;
    }

  } else {
    if (now - lastBuzzerToggle > 150) {
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState);
      lastBuzzerToggle = now;
    }
  }
}

// updateServo()
// ─────────────
// Controls the physical entry barrier gate
// 0°  = gate open  (Safe and Warning — allow entry)
// 90° = gate closed (Critical — block all new entries)
void updateServo() {
  gate.write(alertLevel == 2 ? 90 : 0);
}

// updateLCD()
// ───────────
// Line 1: Alert status text + current risk score
// Line 2: Snapshot of vibration, distance, temperature values
// Called every 500ms to prevent constant flickering from lcd.clear()
void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  if      (alertLevel == 0) lcd.print("SAFE    ");
  else if (alertLevel == 1) lcd.print("WARNING ");
  else                      lcd.print("CRITICAL");
  lcd.print(" R:"); lcd.print((int)riskScore);

  lcd.setCursor(0, 1);
  lcd.print("V:"); lcd.print(lastVib, 1);
  lcd.print(" D:"); lcd.print((int)lastDist);
  lcd.print("c T:"); lcd.print((int)lastTemp);
}

// sendBluetoothAlert()
// ────────────────────
// Sends a formatted multi-line alert to HC-05
// Received on operator's phone via any Bluetooth Serial app
// (e.g. Serial Bluetooth Terminal on Android — pair PIN: 1234)
void sendBluetoothAlert() {
  Serial2.println("==== MOBALERT ====");
  Serial2.print("Status : CRITICAL — EVACUATE");
  Serial2.print(" | Score: "); Serial2.println((int)riskScore);
  Serial2.print("Vib: ");  Serial2.print(lastVib,  2); Serial2.print("G");
  Serial2.print("  Dist: "); Serial2.print((int)lastDist); Serial2.print("cm");
  Serial2.print("  Temp: "); Serial2.print(lastTemp, 1); Serial2.println("C");
  Serial2.println("==================");
}


// ╔══════════════════════════════════════════════════════════════╗
// ║  WEB DASHBOARD                                               ║
// ╚══════════════════════════════════════════════════════════════╝

// handleRoot()
// ────────────
// Serves the complete dashboard HTML page when browser opens ESP32's IP
// Auto-refreshes every 400ms via fetch('/data')
// No external libraries, no CDN — fully self-contained
void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html><html><head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>MobAlert</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#0d0d0d;color:#eee;padding:16px}
h2{font-size:.9em;color:#555;margin-bottom:12px;letter-spacing:.06em;text-transform:uppercase}
#status{font-size:1.5em;font-weight:700;padding:12px 20px;border-radius:10px;margin-bottom:16px;text-align:center;transition:background .4s}
.safe{background:#1b5e20;color:#a5d6a7}
.warning{background:#e65100;color:#ffe0b2}
.critical{background:#b71c1c;color:#ffcdd2;animation:pulse .6s infinite alternate}
@keyframes pulse{from{opacity:1}to{opacity:.75}}
.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(110px,1fr));gap:10px;margin-bottom:16px}
.card{background:#1a1a1a;border:1px solid #222;border-radius:10px;padding:14px;text-align:center}
.card .val{font-size:1.8em;font-weight:700;margin:4px 0}
.card .lbl{font-size:.7em;color:#555;text-transform:uppercase;letter-spacing:.05em}
canvas{width:100%;height:140px;background:#111;border-radius:10px;border:1px solid #1e1e1e;display:block;margin-bottom:8px}
.gl{font-size:.7em;color:#444;text-align:right}
</style></head><body>
<h2>MobAlert — Crowd Crush Early Warning</h2>
<div id='status' class='safe'>Loading...</div>
<div class='cards'>
  <div class='card'><div class='lbl'>Risk Score</div><div class='val' id='sc'>—</div></div>
  <div class='card'><div class='lbl'>Vibration G</div><div class='val' id='vb'>—</div></div>
  <div class='card'><div class='lbl'>Distance cm</div><div class='val' id='ds'>—</div></div>
  <div class='card'><div class='lbl'>Temp C</div><div class='val' id='tp'>—</div></div>
</div>
<canvas id='g'></canvas>
<div class='gl'>Risk score history — last 40 readings</div>
<script>
const cv=document.getElementById('g'),ctx=cv.getContext('2d');
function resize(){cv.width=cv.offsetWidth;cv.height=cv.offsetHeight}
resize();window.addEventListener('resize',resize);
function drawGraph(h){
  const W=cv.width,H=cv.height;
  ctx.clearRect(0,0,W,H);
  ctx.fillStyle='rgba(183,28,28,0.1)';ctx.fillRect(0,0,W,H-(70/100)*H);
  ctx.fillStyle='rgba(230,81,0,0.07)';ctx.fillRect(0,H-(70/100)*H,W,(30/100)*H);
  ctx.strokeStyle='#222';ctx.lineWidth=1;
  [25,50,75].forEach(y=>{
    const py=H-(y/100)*H;
    ctx.beginPath();ctx.moveTo(0,py);ctx.lineTo(W,py);ctx.stroke();
  });
  if(h.length<2)return;
  const step=W/(h.length-1);
  ctx.beginPath();ctx.lineWidth=2.5;ctx.strokeStyle='#4fc3f7';ctx.lineJoin='round';
  h.forEach((v,i)=>{
    const x=i*step,y=H-(v/100)*H;
    i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
  });
  ctx.stroke();
}
function update(){
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('sc').textContent=d.score;
    document.getElementById('vb').textContent=d.vib;
    document.getElementById('ds').textContent=d.dist;
    document.getElementById('tp').textContent=d.temp;
    const s=document.getElementById('status');
    if(d.level===0){s.textContent='SAFE';s.className='safe';}
    else if(d.level===1){s.textContent='WARNING — Crowd building';s.className='warning';}
    else{s.textContent='CRITICAL — EVACUATE NOW';s.className='critical';}
    drawGraph(d.history);
  }).catch(()=>{});
}
setInterval(update,400);
update();
</script></body></html>
)rawhtml";
  server.send(200, "text/html", html);
}

// handleData()
// ────────────
// Returns a JSON object with current sensor readings and history array
// Called by the dashboard JavaScript every 400ms via fetch('/data')
// History array is read from the circular buffer in chronological order
void handleData() {
  String json = "{";
  json += "\"score\":"   + String((int)riskScore) + ",";
  json += "\"level\":"   + String(alertLevel)     + ",";
  json += "\"vib\":"     + String(lastVib,  2)    + ",";
  json += "\"dist\":"    + String((int)lastDist)  + ",";
  json += "\"temp\":"    + String(lastTemp, 1)    + ",";
  json += "\"history\":[";
  for (int i = 0; i < HISTORY_SIZE; i++) {
    int idx = (historyIndex + i) % HISTORY_SIZE;
    json += String((int)scoreHistory[idx]);
    if (i < HISTORY_SIZE - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}
