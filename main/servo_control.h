#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdint.h>

/* ---------------- Continuous Rotation ---------------- */
// Stop the servo (neutral position)
void servo_stop(uint8_t channel);

// Rotate continuously clockwise for given seconds
void servo_rotate_cw(uint8_t channel, float seconds);

// Rotate continuously counter-clockwise for given seconds
void servo_rotate_ccw(uint8_t channel, float seconds);

/* ---------------- Positional Servo ---------------- */
// Move a standard servo to a specific angle (0–180°)
void servo_set_angle(uint8_t channel, float angle_deg);

// Sweep a servo from start_angle → end_angle with step increments
// delay_ms specifies the delay between steps in milliseconds
void servo_sweep(uint8_t channel, float start_angle, float end_angle, float step_deg, int delay_ms);

/* ---------------- Calibration Utility ---------------- */
// Calibrate a servo interactively via serial input (+ / - / s)
void servo_calibrate(uint8_t channel);

#endif // SERVO_CONTROL_H
