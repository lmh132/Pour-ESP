# Pour-ESP

An automated beverage dispensing system powered by an ESP32-S3 microcontroller, designed to control multiple servos and solenoids for precise liquid handling. This system integrates Wi-Fi connectivity to receive mix recipes from a remote server and execute them with hardware precision.

## Overview

Pour-ESP is a barista automation project that controls:
- **PCA9685 PWM Driver**: Manages 16 independent PWM channels for servo and solenoid control
- **Servo Motors**: Support for both positional servos (0–180°) and continuous-rotation servos
- **Solenoid Valves**: High-frequency PWM control for precise liquid dispensing
- **Wi-Fi Connectivity**: Communicates with a remote server to fetch and execute beverage recipes

## Features

### Hardware Control
- **16-Channel PWM Control** via PCA9685 I2C interface
- **Flexible Servo Control**:
  - Positional servo movement (0–180°)
  - Continuous rotation with speed control
  - Servo calibration utilities
  - Sweeping and automated movement patterns
- **Solenoid Valve Control**: Pulse-width modulation for accurate dispensing volumes
- **Configurable PWM Frequencies**: Support for standard servo (50 Hz) and high-frequency solenoid control (200–1500 Hz)

### Software Features
- Built on **ESP-IDF** (Espressif IoT Development Framework)
- **FreeRTOS** for real-time task management
- **Wi-Fi STA Mode**: Connects to Wi-Fi networks for remote control
- **HTTP Client**: Communicates with a remote mix server via POST requests
- **NVS Flash**: Non-volatile storage for configuration persistence

## Project Structure

```
pour/
├── main/                      # Application code
│   ├── pour.c                # Main application entry point
│   ├── servo_control.c/h     # Servo motor control library
│   ├── pca9685.c/h          # PCA9685 PWM driver
│   ├── http_client.c/h      # Wi-Fi and HTTP communication
│   └── CMakeLists.txt       # Component build configuration
├── CMakeLists.txt            # Project build configuration
├── sdkconfig                 # ESP-IDF configuration
└── build/                    # Compiled binaries and build artifacts
```

## Building and Flashing

### Prerequisites
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) (v5.0 or later recommended)
- ESP32-S3 development board
- `idf.py` tool configured in your PATH

### Build
```bash
idf.py build
```

### Flash to Device
```bash
idf.py flash
```

### Monitor Serial Output
```bash
idf.py monitor
```

### Complete Setup (Build, Flash, and Monitor)
```bash
idf.py build flash monitor
```

## API Reference

### Servo Control (`servo_control.h`)

**Continuous Rotation Servos:**
- `servo_stop(channel)` – Stop rotation (neutral position)
- `servo_rotate_cw(channel, seconds)` – Rotate clockwise
- `servo_rotate_ccw(channel, seconds)` – Rotate counter-clockwise

**Positional Servos:**
- `servo_set_angle(channel, angle_deg)` – Set servo to specific angle (0–180°)
- `servo_sweep(channel, start_angle, end_angle, step_deg, delay_ms)` – Sweep servo across range

**Calibration:**
- `servo_calibrate(channel)` – Interactive servo calibration via serial

### PWM Driver (`pca9685.h`)

- `pca9685_init(freq_hz)` – Initialize PCA9685 at specified frequency
  - 50 Hz: Standard RC servos
  - 200–1500 Hz: Solenoids and high-frequency devices

### HTTP Communication (`http_client.h`)

- `wifi_init_sta()` – Initialize Wi-Fi in station mode
- `extend_nozzle(channel, ms)` – Extend a nozzle servo for specified duration
- `solenoid_pulse(channel, ms)` – Pulse a solenoid valve
- `call_mix_endpoint()` – Fetch recipe from remote server and execute

**Remote Server Response Format:**
```json
{
  "status": 1,
  "recipe": [
    {"port": 1, "volume_ml": 250},
    {"port": 2, "volume_ml": 50}
  ]
}
```

## Configuration

### Wi-Fi Credentials
Edit `main/http_client.c` to set your Wi-Fi SSID and password:
```c
#define SSID "your_ssid"
#define PASS "your_password"
```

### PCA9685 Frequency
In `main/pour.c`, adjust the initialization frequency:
```c
pca9685_init(50);  // 50 Hz for servos, or higher for solenoids
```

### Server Endpoint
Configure the remote server URL in `http_client.c` for the `/mix` endpoint.

## Hardware Setup

### Pin Configuration
- **I2C Bus**: SDA and SCL connected to PCA9685 module
- **PCA9685**: 16 PWM outputs (0–15) for servos and solenoids
- **Power**: 5V power supply for PCA9685 and servo motors

### Typical Channel Mapping
- Channels 4–7: Nozzle servos (extend/retract)
- Channels 8–15: Solenoid valves (pour control)
- Channels 0–3: Additional devices as needed

## Usage Example

```c
// Initialize the system
pca9685_init(50);

// Move a servo to 90 degrees
servo_set_angle(0, 90);

// Run continuous rotation for 2 seconds
servo_rotate_cw(1, 2.0);

// Pulse a solenoid for 500ms
solenoid_pulse(8, 500);

// Fetch and execute a recipe from the server
call_mix_endpoint();
```

## License

This project is part of ECE/COMPSCI-655 coursework.

## Author

Leo Hu
