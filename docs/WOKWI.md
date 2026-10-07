# Try it without hardware: Wokwi simulator

[Wokwi](https://wokwi.com) is a free online Arduino simulator. You can run this
exact project in a browser before buying any parts.

## Option A -- wokwi.com (easiest, no install)

1. Open <https://wokwi.com> → **Arduino Uno** → start a new project.
2. Replace the content of `sketch.ino` with [`src/main.ino`](../src/main.ino).
3. Click the small **▾** next to the file tabs → **New file…** → name it
   `monitor_logic.h` and paste [`src/monitor_logic.h`](../src/monitor_logic.h).
4. Open the `diagram.json` tab and replace it with
   [`diagram.json`](../diagram.json) from this repo.
5. Press the green **▶ Start** button.

What to try:

* Turn the potentiometer **down** → green → yellow (below 40 %) → red + beep (below 20 %).
* While it beeps, click the **button** → buzzer is muted, the red LED stays on.
* Turn the knob back **up** → it only returns to green above 45 % (that's the hysteresis).
* Open the Serial Monitor at the bottom to see the CSV log.

## Option B -- Wokwi for VS Code

1. Install the **Wokwi Simulator** extension in VS Code.
2. Build the firmware: `make compile` (needs `arduino-cli` + the `arduino:avr` core).
3. `F1` → **Wokwi: Start Simulator**. The extension reads `wokwi.toml` and
   `diagram.json` from the repo root.

> Note: the parts layout in `diagram.json` is approximate; drag the parts around
> in the editor if wires overlap. The connections are what matters.
