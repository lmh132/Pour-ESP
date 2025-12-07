#include "servo_control.h"
#include "pca9685.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

#define SERVO_PERIOD_MS   20.0f      // 50 Hz
#define PCA9685_STEPS     4096.0f

// Continuous rotation servo tuning
#define SERVO_NEUTRAL_MS  1.5f       // stop
#define SERVO_FORWARD_MS  1.8f       // CW
#define SERVO_REVERSE_MS  1.2f       // CCW

// Standard positional servo range
#define SERVO_MIN_MS      1.0f
#define SERVO_MAX_MS      2.0f

/* ---------------- Internal Helper ---------------- */
// Convert pulse width (ms) → PCA9685 counts
static uint16_t pulse_to_counts(float pulse_ms) {
    if (pulse_ms < 0.5f) pulse_ms = 0.5f;
    if (pulse_ms > 2.5f) pulse_ms = 2.5f;
    return (uint16_t)((pulse_ms / SERVO_PERIOD_MS) * PCA9685_STEPS + 0.5f);
}

// Set raw pulse width for a servo channel
static void servo_set_pulse(uint8_t channel, float pulse_ms) {
    uint16_t off = pulse_to_counts(pulse_ms);
    pca9685_set_pwm(channel, 0, off);
}

/* ---------------- Continuous Rotation ---------------- */
void servo_stop(uint8_t channel) {
    //servo_set_pulse(channel, SERVO_NEUTRAL_MS);
    pca9685_stop_channel(channel);
}

void servo_rotate_cw(uint8_t channel, float seconds) {
    servo_set_pulse(channel, SERVO_FORWARD_MS);
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
    servo_stop(channel);
}

void servo_rotate_ccw(uint8_t channel, float seconds) {
    servo_set_pulse(channel, SERVO_REVERSE_MS);
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
    servo_stop(channel);
}

/* ---------------- Positional Servo ---------------- */
// Convert angle (0–180°) → pulse width (ms)
static float angle_to_pulse(float angle_deg) {
    if (angle_deg < 0.0f) angle_deg = 0.0f;
    if (angle_deg > 180.0f) angle_deg = 180.0f;
    return SERVO_MIN_MS + (angle_deg / 180.0f) * (SERVO_MAX_MS - SERVO_MIN_MS);
}

// Move a standard servo to a specific angle
void servo_set_angle(uint8_t channel, float angle_deg) {
    float pulse = angle_to_pulse(angle_deg);
    servo_set_pulse(channel, pulse);
}

// Sweep a servo between two angles
void servo_sweep(uint8_t channel, float start_angle, float end_angle, float step_deg, int delay_ms) {
    if (start_angle < end_angle) {
        for (float a = start_angle; a <= end_angle; a += step_deg) {
            servo_set_angle(channel, a);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    } else {
        for (float a = start_angle; a >= end_angle; a -= step_deg) {
            servo_set_angle(channel, a);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

/* ---------------- Calibration Utility ---------------- */
void servo_calibrate(uint8_t channel) {
    float pulse = SERVO_NEUTRAL_MS;
    while (1) {
        servo_set_pulse(channel, pulse);
        int c = getchar(); // serial input
        if (c == '+') pulse += 0.01f;
        else if (c == '-') pulse -= 0.01f;
        else if (c == 's') break;

        if (pulse < 1.0f) pulse = 1.0f;
        if (pulse > 2.0f) pulse = 2.0f;
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    servo_set_pulse(channel, pulse);
}
