# Arduino Senior Project Mentoring Demo -- Smart Plant Monitor

A beginner-friendly **Arduino Uno** project and a **12-week mentoring roadmap**
for a high-school senior project, built to show how I would guide a student who
is completely new to electronics -- from "what is a resistor?" to a working,
documented project that satisfies the teacher's project form.


## What it does

* Reads a moisture sensor on **A0** (a potentiometer simulates it).
* **Green** LED = OK, **yellow** = getting dry, **red + beeping buzzer** = water now.
* A **button** (debounced) mutes the alarm.
* **No `delay()`** -- everything is scheduled with `millis()`.
* Logs CSV to the Serial Monitor (`ms,raw,percent,level,muted`) for charts in the report.

## Repository layout

```
src/main.ino              Sketch -- hardware only, heavily commented
src/monitor_logic.h       Pure C++ logic: calibration, smoothing, timer, debounce, alert state machine
tests/test_monitor_logic.cpp  Host-side unit tests (g++, no board needed)
diagram.json              Wokwi simulator circuit
wokwi.toml                Wokwi for VS Code config
platformio.ini            Optional PlatformIO build
docs/WIRING.md            Parts list, pin table, ASCII + Mermaid diagrams, Ohm's law
docs/WOKWI.md             Run it in the browser without hardware
docs/SENIOR_PROJECT_ROADMAP.md  Week-by-week milestones + mapping to the teacher's form
```

## Run it

**In the browser (no hardware):** follow [docs/WOKWI.md](docs/WOKWI.md) -- paste
`main.ino`, `monitor_logic.h` and `diagram.json` into a new Wokwi Uno project and press ▶.

**Arduino IDE:**
1. Create a sketch folder called `main`, copy `src/main.ino` and `src/monitor_logic.h` into it.
2. Open `main.ino`, select **Tools → Board → Arduino Uno** and the right port.
3. Upload, then open the Serial Monitor at **9600** baud.

**PlatformIO:** `pio run -t upload && pio device monitor`

**Command line compile check:** `make compile` (needs `arduino-cli` and `arduino-cli core install arduino:avr`).

## Run the tests

The decision logic is plain C++, so it can be tested on any computer:

```bash
make test
# ...
# 59 checks, 0 failures
# ALL TESTS PASSED
```

Both checks (unit tests + `make compile` for `arduino:avr:uno`) are plain commands,
so they drop straight into any CI system.

## How I'd mentor this project

I'm **Mihai Chindris**, a software engineer (BSc Computer Science, Technical
University of Cluj-Napoca, 2019; MSc Business Informatics at TU Wien in progress).
I enjoy breaking hardware and software topics down for beginners.

* **Short weekly live sessions (30–45 min).** One concept per week, hands-on:
  you build, I guide. We follow [the roadmap](docs/SENIOR_PROJECT_ROADMAP.md)
  but adjust the pace to your schedule.
* **Async help between sessions.** Stuck on a wiring photo or a compile error at
  9 pm? Send it -- I'll reply with a hint, not just the answer, so you still own the project.
* **Safety first.** Only USB/low-voltage parts; we cover safe habits before touching anything.
* **Simulator before shopping.** We prototype in Wokwi so you only buy parts you need.
* **Your teacher's form drives the milestones.** In the first session we go through
  the form together and map every section to a deliverable (see the table in the roadmap),
  so documentation grows week by week instead of being a last-minute rush.
* **You present it, you understand it.** By the end you can explain every line of
  `main.ino` and every wire on the breadboard.

## License

[MIT](LICENSE)
