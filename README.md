
# Arduino Funshield Drivers

A collection of low-level drivers and higher-level utility classes for **Arduino Funshield**.
The project provides a unified abstraction layer for controlling the 4-digit 7-segment display, LEDs, and buttons, along with ready-to-use components such as a stopwatch, counter, and scrolling text module.


### `driver.ino`

### Low-Level Drivers

Core classes that directly control Funshield hardware:

* **Display / NumericDisplay / TextDisplay**
  Control of the 4-digit 7-segment display:
  − digit rendering
  − custom glyphs
  − decimal point
  − multiplexed per-digit refresh cycle

* **Button**
  State tracking and event detection (press/hold).
  Adjustable timing logic via an internal `Timer`.

* **Diode**
  Simple LED control (on/off, toggle).

* **Timer**
  A wrapper on top of `millis()` for periodic events.

### High-Level Components

Modules built on top of the driver layer:

* **Stopwatch**
  Start/stop, freeze/unfreeze (lap), reset
  Automatic display formatting for the 7-segment display

* **Counter**
  A multi-digit positional counter with increment/decrement of the active digit.

* **RunningMessage**
  A scrolling text engine that uses `SerialInputHandler` as the input source.



### `funshield.h`

Defines all pin assignments for Funshield, digit glyphs, and helper constants.



