#ifndef PCA9685_H
#define PCA9685_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================
// PCA9685 Unified Driver Header
// Supports: 
//   • Arbitrary PWM frequency (up to ~1600 Hz)
//   • High-frequency PWM for solenoids
//   • Standard 50 Hz servo control
//   • Continuous-rotation servo speed control
// =============================================================

// PCA9685 resolution
#define PCA9685_STEPS 4096

// Servo pulse limits (typical)
#define SERVO_MIN_PULSE 0.55f   // ms
#define SERVO_MAX_PULSE 2.45f   // ms

// -------------------------------------------------------------
// Initialize PCA9685 at given PWM frequency
// freq_hz examples:
//     50    – Standard RC servos
//     200–1500 – Solenoids, motors, LEDs
// -------------------------------------------------------------
void pca9685_init(uint16_t freq_hz);

// -------------------------------------------------------------
// Set raw PWM values for a channel
// "on" and "off" are 12-bit (0–4095)
// -------------------------------------------------------------
void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off);

// -------------------------------------------------------------
// Set duty cycle (0–100%) for a channel
// -------------------------------------------------------------
void pca9685_set_duty(uint8_t channel, float duty_percent);

// -------------------------------------------------------------
// Set standard servo angle (0–180°)
// NOTE: PCA must be initialized at 50 Hz.
// -------------------------------------------------------------
void pca9685_set_servo_angle(uint8_t channel, float angle_deg);

// -------------------------------------------------------------
// Continuous-rotation servo control
// speed = -1.0 (full CCW) to +1.0 (full CW)
// NOTE: PCA must be initialized at 50 Hz.
// -------------------------------------------------------------
void pca9685_set_servo_speed(uint8_t channel, float speed);

void pca9685_stop_channel(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif // PCA9685_H
