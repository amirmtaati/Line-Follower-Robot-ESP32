# ESP32 Line Follower Robot

A line follower robot built on ESP32 with FreeRTOS, featuring a PID controller, IR sensor array, TB6612FNG motor driver, and a live web dashboard for monitoring and tuning over WiFi.

This started as a port of an Arduino UNO line follower to ESP32 with FreeRTOS and grew into a full embedded systems project covering concurrent task architecture, sensor calibration, PID control, motor driving, and a real-time web interface.

## Hardware

| Component | Details |
|---|---|
| Microcontroller | ESP32 DevKit |
| Motor driver | TB6612FNG |
| IR sensors | TCRT5000 breakout boards x 4 |
| Motors | DC gear motors x 2 |
| Indicators | RGB LED (3 pins) + buzzer + push button |

### Wiring

**IR Sensors** (analog output, with 10kΩ pull-down resistors on OUT pins)

| Sensor | GPIO |
|---|---|
| IR0 | 32 |
| IR1 | 33 |
| IR2 | 34 |
| IR3 | 35 |

**TB6612FNG Motor Driver**

| Signal | GPIO |
|---|---|
| PWMA | 27 |
| AIN1 | 25 |
| AIN2 | 26 |
| PWMB | 23 |
| BIN1 | 18 |
| BIN2 | 19 |
| STBY | 5 |

**Other**

| Component | GPIO |
|---|---|
| Red LED | 12 |
| Green LED | 14 |
| Blue LED | 22 |
| Buzzer | 21 |
| Button | 15 |

The IR sensors are powered from the ESP32's 3.3V pin. A 10kΩ pull-down resistor on each sensor's OUT pin is required to get proper analog readings. Without it you get digital-only behavior (0 or max) regardless of surface.

## Software Architecture

The firmware runs on FreeRTOS with a queue-based sensor pipeline and mutex-protected shared state.

```
vReadSensorValuesTask
        |
        |  irValuesQ (size 1, xQueueOverwrite)
        v
vNormalizeSensorValuesTask
        |
        |  normalizedSensorValuesQ (size 1, xQueueOverwrite)
        v
vRobotTask  <--  vButtonTask
                 vWiFiTask
                 vTelemetryTask
```

| Task | Priority | Role |
|---|---|---|
| vReadSensorValuesTask | 3 | Reads ADC every 2ms |
| vNormalizeSensorValuesTask | 3 | Normalizes sensor values |
| vButtonTask | 2 | Debounces button, changes state |
| vRobotTask | 2 | PID control, motor output, calibration |
| vWiFiTask | 1 | Connects to WiFi, starts web server |
| vTelemetryTask | 1 | Pushes SSE events to dashboard |

All shared state lives in a single `RobotState` struct protected by `robotMutex`. Queues are size-1 with `xQueueOverwrite` so the robot always acts on the freshest sensor data rather than queued-up stale readings.

## Robot States

The robot has three states, cycled by the push button or the web dashboard.

```
STOPPED -> (button press, not calibrated) -> CALIBRATING -> STOPPED
STOPPED -> (button press, calibrated)     -> MOVING
MOVING  -> (button press)                 -> STOPPED
```

LED colors: red for STOPPED, blue for CALIBRATING, green for MOVING.

## Calibration

Calibration runs a two-phase routine to capture white and black surface readings.

1. **White phase** (2 seconds, high buzzer tone) — place all sensors over the white surface
2. **3 second gap** — move the robot so all sensors are over the black line
3. **Black phase** (2 seconds, low buzzer tone) — hold still over the black line

Values are averaged over each window and stored in `WHITE_VALUES[]` and `BLACK_VALUES[]`. The normalization formula maps white to 0.0 and black to 1.0.

During calibration, `vNormalizeSensorValuesTask` yields completely via the `isCalibrating` flag so the calibration function has exclusive access to `irValuesQ`. Without this, both tasks race for the same queue and calibration collects no data.

## PID Controller

The error signal is a weighted centroid of the normalized sensor values:

```
error = sum(weight[i] * normalized[i]) / sum(normalized[i])
```

Weights are `{-3.0, -1.0, 1.0, 3.0}` where negative is left and positive is right. The correction is:

```
correction = (kP * error) + (kD * derivative)
derivative = error - last_error

left_pwm  = base_speed - correction
right_pwm = base_speed + correction
```

PWM is 12-bit (0 to 4095) using the ESP32 LEDC peripheral at 5kHz.

## Web Dashboard

Once connected to WiFi, the robot hosts a web server at its IP address, printed to serial on boot. Open it in any browser on the same network.

The dashboard streams live telemetry via Server-Sent Events with a 200ms update rate, falling back to HTTP polling if SSE is unavailable. It shows robot state, error, correction, normalized sensor values, calibration values per sensor, and left/right motor PWM.

kP, kD, and base speed can be set from the browser and applied immediately without reflashing. The dashboard also has START, STOP, and CALIBRATE buttons so the robot can be controlled entirely over WiFi once it is on the track.

## Project Structure

```
include/
  config.hh          pin declarations
  types.hh           shared state declarations (RobotState, queues, mutexes)
  sensors.hh
  motor.hh
  robot.hh
  utils.hh
  wifi.hh
  wifi_driver.hh     ESP-IDF WiFi abstraction interface
  debug.hh
  credentials.hh     WiFi credentials (gitignored)

src/
  config.cpp         pin definitions
  types.cpp          shared state definitions and initial values
  sensors.cpp        sensor tasks and calibration
  motor.cpp          TB6612 motor driver
  robot.cpp          main robot task, PID loop
  utils.cpp          error calculation, button task
  wifi.cpp           web server, routes, HTML dashboard, telemetry task
  wifi_driver.cpp    raw ESP-IDF WiFi initialization, isolated here only
  debug.cpp          serial debug printing
  main.cpp           setup and task creation
```

The WiFi layer is split into two files intentionally. `wifi_driver.cpp` is the only file that touches raw ESP-IDF. Everything else calls `wifiConnect()`, `wifiIPAddress()`, and `wifiIsConnected()` through the interface in `wifi_driver.hh`. If Espressif changes their API in a future SDK version, only one file needs updating.

## Building

The project uses PlatformIO.

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    ottowinter/ESPAsyncWebServer-esphome @ ^3.1.0
    ottowinter/AsyncTCP-esphome @ ^1.2.1
```

Before building, create `include/credentials.hh` with your WiFi credentials:

```cpp
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID     "your_network_name"
#define WIFI_PASSWORD "your_password"

#endif
```

Add it to `.gitignore` so it never gets committed:

```
include/credentials.hh
```

Then build and flash:

```bash
pio run --target upload
pio device monitor
```

The serial monitor will print the IP address once WiFi connects.

## Notes

The ESP32 ADC on GPIO 34 and 35 is input-only with no internal pullup. This is fine for sensors but those pins cannot be used for any output.

LEDC channels 0, 2, and 4 are used for buzzer, motor A, and motor B respectively. These were chosen to avoid timer conflicts — channels on the same timer share frequency and resolution, which causes one to interfere with the other if they need different settings.

`setupMotors()` must be called last in `setup()`, after all other initialization. Calling it earlier causes LEDC initialization to race with GPIO setup and the system freezes on boot.

WiFi uses raw ESP-IDF instead of the Arduino `WiFi.h` wrapper because the Arduino wrapper produced `STA enable failed` errors in this particular FreeRTOS configuration.