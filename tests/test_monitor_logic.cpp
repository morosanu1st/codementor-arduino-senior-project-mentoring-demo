// Host-side unit tests for src/monitor_logic.h
// Build & run:  make test
// No framework needed: a tiny CHECK macro counts failures.
#include <cstdio>

#include "monitor_logic.h"

using namespace monitor;

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    ++g_checks;                                                           \
    if (!(cond)) {                                                        \
      ++g_failures;                                                       \
      std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
    }                                                                     \
  } while (0)

static void test_raw_to_percent() {
  std::printf("rawToPercent\n");
  CHECK(rawToPercent(0, 0, 1023) == 0);
  CHECK(rawToPercent(1023, 0, 1023) == 100);
  CHECK(rawToPercent(512, 0, 1023) == 50);
  // inverted capacitive sensor: 600 = dry (0 %), 280 = wet (100 %)
  CHECK(rawToPercent(600, 600, 280) == 0);
  CHECK(rawToPercent(280, 600, 280) == 100);
  CHECK(rawToPercent(440, 600, 280) == 50);
  // clamping
  CHECK(rawToPercent(700, 600, 280) == 0);
  CHECK(rawToPercent(100, 600, 280) == 100);
  // bad calibration does not crash
  CHECK(rawToPercent(500, 300, 300) == 0);
}

static void test_moving_average() {
  std::printf("MovingAverage\n");
  MovingAverage<4> avg;
  CHECK(avg.add(100) == 100);   // only one sample so far
  CHECK(avg.add(200) == 150);
  CHECK(avg.add(300) == 200);
  CHECK(avg.add(400) == 250);
  CHECK(avg.count() == 4);
  CHECK(avg.add(500) == 350);   // oldest (100) dropped: (200+300+400+500)/4
}

static void test_interval_timer() {
  std::printf("IntervalTimer\n");
  IntervalTimer t(100);
  t.reset(0);
  CHECK(!t.due(50));
  CHECK(t.due(100));
  CHECK(!t.due(150));
  CHECK(t.due(200));
  // millis() overflow: 0xFFFFFFF0 + 0x20 wraps around to 0x10
  IntervalTimer w(30);
  w.reset(0xFFFFFFF0UL);
  CHECK(!w.due(0xFFFFFFFFUL));
  CHECK(w.due(0x00000010UL));
}

static void test_debouncer() {
  std::printf("Debouncer\n");
  Debouncer b(30);
  // bouncing contacts: rapid on/off should not trigger
  CHECK(!b.update(true, 0));
  CHECK(!b.update(false, 5));
  CHECK(!b.update(true, 10));
  CHECK(!b.update(true, 30));    // only 20 ms stable so far
  CHECK(b.update(true, 40));     // 30 ms stable -> one press event
  CHECK(b.isPressed());
  CHECK(!b.update(true, 100));   // holding the button: no repeat events
  CHECK(!b.update(false, 110));  // release starts its own debounce
  CHECK(!b.update(false, 140));  // release accepted, but no "press" event
  CHECK(!b.isPressed());
  CHECK(!b.update(true, 200));
  CHECK(b.update(true, 230));    // second real press
}

static void test_levels_and_hysteresis() {
  std::printf("classify / nextLevel\n");
  const Thresholds t = {40, 20, 5};
  CHECK(classify(80, t) == LEVEL_OK);
  CHECK(classify(40, t) == LEVEL_OK);
  CHECK(classify(39, t) == LEVEL_WARNING);
  CHECK(classify(20, t) == LEVEL_WARNING);
  CHECK(classify(19, t) == LEVEL_ALARM);

  // getting worse is immediate
  CHECK(nextLevel(LEVEL_OK, 39, t) == LEVEL_WARNING);
  CHECK(nextLevel(LEVEL_OK, 10, t) == LEVEL_ALARM);
  // getting better needs the extra 5 %
  CHECK(nextLevel(LEVEL_WARNING, 41, t) == LEVEL_WARNING);
  CHECK(nextLevel(LEVEL_WARNING, 45, t) == LEVEL_OK);
  CHECK(nextLevel(LEVEL_ALARM, 22, t) == LEVEL_ALARM);
  CHECK(nextLevel(LEVEL_ALARM, 25, t) == LEVEL_WARNING);
  CHECK(nextLevel(LEVEL_ALARM, 60, t) == LEVEL_OK);
  CHECK(levelName(LEVEL_ALARM)[0] == 'A');
}

static void test_alert_controller() {
  std::printf("AlertController\n");
  const Thresholds t = {40, 20, 5};
  AlertController a(t);
  a.update(80);
  Outputs o = a.outputs(0);
  CHECK(o.green && !o.yellow && !o.red && !o.buzzer);

  a.toggleMute();                 // muting while OK does nothing
  CHECK(!a.muted());

  a.update(30);
  o = a.outputs(0);
  CHECK(!o.green && o.yellow && !o.red && !o.buzzer);

  a.update(10);
  CHECK(a.level() == LEVEL_ALARM);
  CHECK(a.outputs(100).buzzer);   // inside the 200 ms "on" window
  CHECK(!a.outputs(500).buzzer);  // in the "off" part of the beep
  CHECK(a.outputs(1150).buzzer);  // next second, on again
  CHECK(a.outputs(100).red);

  a.toggleMute();
  CHECK(a.muted());
  CHECK(!a.outputs(100).buzzer);  // muted: red LED stays, buzzer silent
  CHECK(a.outputs(100).red);

  a.update(70);                   // watered the plant
  CHECK(a.level() == LEVEL_OK);
  CHECK(!a.muted());              // auto un-mute for the next alarm
}

int main() {
  test_raw_to_percent();
  test_moving_average();
  test_interval_timer();
  test_debouncer();
  test_levels_and_hysteresis();
  test_alert_controller();
  std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
