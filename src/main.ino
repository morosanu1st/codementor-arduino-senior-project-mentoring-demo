// =============================================================================
// Smart Plant Monitor -- Arduino Uno beginner project
// =============================================================================
// What it does
//   * Reads a moisture sensor on A0 (a potentiometer stands in for it in the
//     Wokwi simulator -- turn the knob to "dry out" the plant).
//   * Green LED  = soil is fine
//     Yellow LED = getting dry (WARNING)
//     Red LED + beeping buzzer = water now! (ALARM)
//   * The push button mutes the buzzer while an alarm is active.
//   * Prints a CSV line to the Serial Monitor every second, so you can paste
//     the data into a spreadsheet for your project report.
//
// Big idea: there is NO delay() in loop(). Everything is scheduled with
// millis(), so the button stays responsive while the sensor is being read.
//
// All decision-making lives in monitor_logic.h (unit-tested on a PC).
// This file only talks to the hardware.
// =============================================================================

#include "monitor_logic.h"

// ---------------- Pin assignments (match docs/WIRING.md) ---------------------
const uint8_t PIN_SENSOR = A0;     // analog input: soil sensor / potentiometer
const uint8_t PIN_BUTTON = 2;      // push button to GND (uses internal pull-up)
const uint8_t PIN_LED_GREEN = 5;   // each LED goes through a 220 ohm resistor
const uint8_t PIN_LED_YELLOW = 6;
const uint8_t PIN_LED_RED = 7;
const uint8_t PIN_BUZZER = 8;      // piezo buzzer

// ---------------- Calibration ------------------------------------------------
// With a potentiometer: 0 = 0 %, 1023 = 100 %.
// With a capacitive soil sensor, measure it yourself: note the reading in dry
// air (e.g. 600) and in a glass of water (e.g. 280), then put those here.
const int RAW_AT_0_PERCENT = 0;
const int RAW_AT_100_PERCENT = 1023;

// ---------------- Thresholds -------------------------------------------------
// warn below 40 %, alarm below 20 %, need +5 % to go back to a better level.
const monitor::Thresholds THRESHOLDS = {40, 20, 5};

const uint16_t BUZZER_FREQUENCY_HZ = 2000;

// ---------------- Helper objects from monitor_logic.h ------------------------
monitor::IntervalTimer sampleTimer(100);   // read the sensor 10 times a second
monitor::IntervalTimer logTimer(1000);     // print a log line once a second
monitor::Debouncer button(30);             // 30 ms debounce
monitor::MovingAverage<8> smoother;        // average of the last 8 readings
monitor::AlertController alert(THRESHOLDS);

// Values shared between the parts of loop()
int lastRaw = 0;
int lastPercent = 100;
bool buzzerOn = false;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);  // pressed = LOW, released = HIGH
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);
  Serial.println(F("# Smart Plant Monitor started"));
  Serial.println(F("ms,raw,percent,level,muted"));  // CSV header
}

void loop() {
  uint32_t now = millis();

  // 1) Button: INPUT_PULLUP means a pressed button reads LOW.
  bool rawPressed = (digitalRead(PIN_BUTTON) == LOW);
  if (button.update(rawPressed, now)) {
    alert.toggleMute();
    Serial.print(F("# button pressed, muted="));
    Serial.println(alert.muted() ? F("yes") : F("no"));
  }

  // 2) Sensor: sample, smooth, convert to %, update the alert state.
  if (sampleTimer.due(now)) {
    lastRaw = analogRead(PIN_SENSOR);
    int smoothed = smoother.add(lastRaw);
    lastPercent = monitor::rawToPercent(smoothed, RAW_AT_0_PERCENT, RAW_AT_100_PERCENT);
    alert.update(lastPercent);
  }

  // 3) Outputs: ask the logic what the LEDs and buzzer should do right now.
  monitor::Outputs out = alert.outputs(now);
  digitalWrite(PIN_LED_GREEN, out.green ? HIGH : LOW);
  digitalWrite(PIN_LED_YELLOW, out.yellow ? HIGH : LOW);
  digitalWrite(PIN_LED_RED, out.red ? HIGH : LOW);

  // Only call tone()/noTone() when the state changes (avoids restarting it).
  if (out.buzzer != buzzerOn) {
    buzzerOn = out.buzzer;
    if (buzzerOn) {
      tone(PIN_BUZZER, BUZZER_FREQUENCY_HZ);
    } else {
      noTone(PIN_BUZZER);
    }
  }

  // 4) Logging: one CSV line per second.
  if (logTimer.due(now)) {
    Serial.print(now);
    Serial.print(',');
    Serial.print(lastRaw);
    Serial.print(',');
    Serial.print(lastPercent);
    Serial.print(',');
    Serial.print(monitor::levelName(alert.level()));
    Serial.print(',');
    Serial.println(alert.muted() ? F("yes") : F("no"));
  }
}
