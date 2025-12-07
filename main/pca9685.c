#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_MASTER_SCL_IO 9
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MASTER_NUM I2C_NUM_0

#define PCA9685_ADDR 0x40
#define PCA9685_STEPS 4096

// Servo pulse range (ms)
#define SERVO_MIN_PULSE 0.55f
#define SERVO_MAX_PULSE 2.45f

//---------------------------------------------
// Low-level: write 8-bit register
//---------------------------------------------
static void pca9685_write8(uint8_t reg, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PCA9685_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
}

//---------------------------------------------
// Initialize PCA9685 at any frequency
//---------------------------------------------
void pca9685_init(uint16_t freq_hz)
{
    // --- I2C init ---
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // --- Setup prescale ---
    pca9685_write8(0x00, 0x10);   // MODE1 sleep

    uint8_t prescale = (uint8_t)(25000000.0f / (PCA9685_STEPS * freq_hz) - 1.0f);
    pca9685_write8(0xFE, prescale);   // PRESCALE register

    // --- Wake up ---
    pca9685_write8(0x00, 0x00);  // wake
    vTaskDelay(pdMS_TO_TICKS(5));

    pca9685_write8(0x00, 0xA1);  // restart + auto-increment
}

//---------------------------------------------
// Set PWM using raw registers
//---------------------------------------------
void pca9685_set_pwm(uint8_t channel, uint16_t on, uint16_t off)
{
    uint8_t reg = 0x06 + 4 * channel;
    uint8_t data[5] = {
        reg,
        on & 0xFF,
        on >> 8,
        off & 0xFF,
        off >> 8
    };
    i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_ADDR, data, 5, pdMS_TO_TICKS(100));
}

//---------------------------------------------
// Set duty cycle 0–100% (general PWM)
//---------------------------------------------
void pca9685_set_duty(uint8_t channel, float duty_percent)
{
    if (duty_percent < 0) duty_percent = 0;
    if (duty_percent > 100) duty_percent = 100;

    uint16_t off = (uint16_t)((duty_percent / 100.0f) * (PCA9685_STEPS - 1));

    pca9685_set_pwm(channel, 0, off);
}

//---------------------------------------------
// Set servo angle 0–180° (requires 50 Hz init)
//---------------------------------------------
void pca9685_set_servo_angle(uint8_t channel, float angle_deg)
{
    if (angle_deg < 0) angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;

    float pulse_ms = SERVO_MIN_PULSE +
                     (angle_deg / 180.0f) * (SERVO_MAX_PULSE - SERVO_MIN_PULSE);

    // 20 ms period at 50 Hz
    uint16_t off = (uint16_t)((pulse_ms / 20.0f) * PCA9685_STEPS);

    pca9685_set_pwm(channel, 0, off);
}

// ---------------------------------------------
// Continuous-rotation servo control
// speed = -1.0 (full CCW)  to  +1.0 (full CW)
// ---------------------------------------------
void pca9685_set_servo_speed(uint8_t channel, float speed)
{
    if (speed > 1.0f) speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    // 1.5 ms = stop
    float pulse_ms = 1.5f;

    if (speed > 0) {
        // CW: 1.5 → SERVO_MAX_PULSE
        pulse_ms = 1.5f + speed * (SERVO_MAX_PULSE - 1.5f);
    } else if (speed < 0) {
        // CCW: 1.5 → SERVO_MIN_PULSE
        pulse_ms = 1.5f + speed * (1.5f - SERVO_MIN_PULSE);
    }

    // convert ms → PCA9685 counts
    uint16_t off = (uint16_t)((pulse_ms / 20.0f) * PCA9685_STEPS);

    pca9685_set_pwm(channel, 0, off);
}

void pca9685_stop_channel(uint8_t channel)
{
    uint8_t reg = 0x06 + 4 * channel + 3;  // LEDx_OFF_H register
    pca9685_write8(reg, 0x10);             // FULL OFF bit
}