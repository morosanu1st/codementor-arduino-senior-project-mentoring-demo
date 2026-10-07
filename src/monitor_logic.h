// monitor_logic.h
// -----------------------------------------------------------------------------
// Pure C++ "brain" of the Smart Plant Monitor.
//
// Why a separate file?
//   * Nothing in here touches Arduino hardware (no digitalRead, no millis()).
//     The sketch passes in numbers (sensor values, current time) and gets
//     decisions back.
//   * That means we can compile and unit-test this logic on a normal computer
//     with g++ (see tests/test_monitor_logic.cpp) -- no board required.
//   * It is a habit used in professional embedded work: keep hardware code thin,
//     keep logic testable.
//
// Only <stdint.h> is used so the same file compiles for the Arduino Uno (AVR)
// and for a desktop PC.
// -----------------------------------------------------------------------------
#ifndef MONITOR_LOGIC_H
#define MONITOR_LOGIC_H

#include <stdint.h>

namespace monitor {

// -----------------------------------------------------------------------------
// 1) Converting a raw sensor reading into a percentage
// -----------------------------------------------------------------------------
// analogRead() returns 0..1023. Real sensors rarely use the full range, so we
// "calibrate": rawAtZero is the reading that means 0 %, rawAtHundred the reading
// that means 100 %. Capacitive soil sensors are inverted (dry = HIGH number),
// so rawAtZero may be bigger than rawAtHundred -- the math below handles both.
inline int rawToPercent(int raw, int rawAtZero, int rawAtHundred) {
  if (rawAtZero == rawAtHundred) {
    return 0;  // bad calibration, avoid dividing by zero
  }
  long percent = (long)(raw - rawAtZero) * 100L / (long)(rawAtHundred - rawAtZero);
  if (percent < 0) percent = 0;      // clamp: never below 0 %
  if (percent > 100) percent = 100;  // clamp: never above 100 %
  return (int)percent;
}

// -----------------------------------------------------------------------------
// 2) Smoothing noisy readings with a moving average
// -----------------------------------------------------------------------------
// Analog readings jitter by a few counts. Averaging the last N samples gives a
// calmer value so the LEDs do not flicker between states.
template <int N>
class MovingAverage {
 public:
  MovingAverage() : sum_(0), count_(0), index_(0) {
    for (int i = 0; i < N; ++i) buffer_[i] = 0;
  }

  // Add a new sample and return the current average.
  int add(int sample) {
    sum_ -= buffer_[index_];      // forget the oldest sample
    buffer_[index_] = sample;     // store the new one in its place
    sum_ += sample;
    index_ = (index_ + 1) % N;    // move around the ring buffer
    if (count_ < N) ++count_;     // until the buffer is full, divide by count_
    return (int)(sum_ / count_);
  }

  int count() const { return count_; }

 private:
  int buffer_[N];
  long sum_;
  int count_;
  int index_;
};

// -----------------------------------------------------------------------------
// 3) Non-blocking timing (the millis() pattern)
// -----------------------------------------------------------------------------
// delay() freezes the whole program. Instead we ask "has enough time passed?"
// on every loop. Using unsigned subtraction (now - last) keeps working even
// when millis() wraps around to 0 after ~49.7 days.
class IntervalTimer {
 public:
  explicit IntervalTimer(uint32_t intervalMs) : interval_(intervalMs), last_(0) {}

  // Returns true once every intervalMs milliseconds.
  bool due(uint32_t nowMs) {
    if ((uint32_t)(nowMs - last_) >= interval_) {
      last_ = nowMs;
      return true;
    }
    return false;
  }

  void reset(uint32_t nowMs) { last_ = nowMs; }

 private:
  uint32_t interval_;
  uint32_t last_;
};

// -----------------------------------------------------------------------------
// 4) Button debouncing
// -----------------------------------------------------------------------------
// A mechanical button "bounces": for a few milliseconds it flickers between
// pressed and released. We only accept a new state after it has been stable
// for delayMs. update() returns true exactly once per real press.
class Debouncer {
 public:
  explicit Debouncer(uint32_t delayMs = 30)
      : delay_(delayMs), lastRaw_(false), stable_(false), lastChange_(0) {}

  bool update(bool rawPressed, uint32_t nowMs) {
    if (rawPressed != lastRaw_) {   // input changed -> restart the stability clock
      lastRaw_ = rawPressed;
      lastChange_ = nowMs;
    }
    if ((uint32_t)(nowMs - lastChange_) >= delay_ && stable_ != lastRaw_) {
      stable_ = lastRaw_;           // it has been steady long enough: accept it
      return stable_;               // report only the press (not the release)
    }
    return false;
  }

  bool isPressed() const { return stable_; }

 private:
  uint32_t delay_;
  bool lastRaw_;
  bool stable_;
  uint32_t lastChange_;
};

// -----------------------------------------------------------------------------
// 5) Alert levels with hysteresis (a tiny state machine)
// -----------------------------------------------------------------------------
// Order matters: a higher number means "worse".
enum Level : uint8_t { LEVEL_OK = 0, LEVEL_WARNING = 1, LEVEL_ALARM = 2 };

struct Thresholds {
  int warnBelow;   // below this % -> WARNING
  int alarmBelow;  // below this % -> ALARM
  int hysteresis;  // extra % needed before we go back to a better level
};

inline const char* levelName(Level level) {
  switch (level) {
    case LEVEL_OK: return "OK";
    case LEVEL_WARNING: return "WARNING";
    case LEVEL_ALARM: return "ALARM";
  }
  return "?";
}

// Plain classification, no memory of the past.
inline Level classify(int percent, const Thresholds& t) {
  if (percent < t.alarmBelow) return LEVEL_ALARM;
  if (percent < t.warnBelow) return LEVEL_WARNING;
  return LEVEL_OK;
}

// Hysteresis: getting worse happens immediately, getting better only once the
// value is clearly past the threshold. Without this, a reading sitting right on
// the line would make the buzzer chatter on and off.
inline Level nextLevel(Level current, int percent, const Thresholds& t) {
  Level target = classify(percent, t);
  if (target >= current) {
    return target;  // same or worse: switch right away
  }
  if (current == LEVEL_ALARM) {
    if (percent < t.alarmBelow + t.hysteresis) return LEVEL_ALARM;
    return (percent >= t.warnBelow + t.hysteresis) ? LEVEL_OK : LEVEL_WARNING;
  }
  // current == LEVEL_WARNING and the target is OK
  return (percent >= t.warnBelow + t.hysteresis) ? LEVEL_OK : LEVEL_WARNING;
}

// -----------------------------------------------------------------------------
// 6) Putting it together: what should the LEDs and buzzer do?
// -----------------------------------------------------------------------------
struct Outputs {
  bool green;
  bool yellow;
  bool red;
  bool buzzer;
};

class AlertController {
 public:
  explicit AlertController(const Thresholds& t)
      : thresholds_(t), level_(LEVEL_OK), muted_(false) {}

  // Feed the latest (smoothed) percentage.
  void update(int percent) {
    level_ = nextLevel(level_, percent, thresholds_);
    if (level_ == LEVEL_OK) {
      muted_ = false;  // problem solved -> un-mute so the next alarm is heard
    }
  }

  // The button calls this. Muting only makes sense while an alarm is active.
  void toggleMute() {
    if (level_ == LEVEL_ALARM) muted_ = !muted_;
  }

  Level level() const { return level_; }
  bool muted() const { return muted_; }

  // Beep pattern while alarming: 200 ms on, 800 ms off (every second).
  Outputs outputs(uint32_t nowMs) const {
    Outputs o;
    o.green = (level_ == LEVEL_OK);
    o.yellow = (level_ == LEVEL_WARNING);
    o.red = (level_ == LEVEL_ALARM);
    o.buzzer = (level_ == LEVEL_ALARM) && !muted_ && (nowMs % 1000UL) < 200UL;
    return o;
  }

 private:
  Thresholds thresholds_;
  Level level_;
  bool muted_;
};

}  // namespace monitor

#endif  // MONITOR_LOGIC_H
