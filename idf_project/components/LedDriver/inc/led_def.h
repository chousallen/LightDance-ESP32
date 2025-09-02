#pragma once
#include <stdint.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==== I2C (ESP32) ====
// Using I2C_NUM_0 on default pins (SDA=21, SCL=22).
#define I2C_PORT I2C_NUM_0
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_TRANSMIT_TIMEOUT -1 //wait timeout, in ms. -1 means wait forever
#define I2C_SCL_FREQ 100000 //adjust to 1000000 for Fm+ mode

// ==== Limits ====
// Total logical channels your app supports (across all backends).
#define MAX_CHANNEL_NUM 48

// Number of PCA9955B devices expected on the bus (each PCA9955B device can control 5 LEDs/channels).
#define MAX_PCA9955_NUM 8

// ---- Sanity checks ----
#if (MAX_PCA9955_NUM) < 0
#error "MAX_PCA9955_NUM must be >= 0"
#endif

#if (MAX_CHANNEL_NUM) <= 0
#error "MAX_CHANNEL_NUM must be > 0"
#endif

// ==== Color ====
// NOTE: GRB byte order to match WS2812/NeoPixel expectations.
typedef struct {
    uint8_t green;
    uint8_t red;
    uint8_t blue;
} color_t;

// ==== LED types ====
//   LED_TYPE_OF       = PCA9955B device / a discrete GPIO LED or similar on/off/dimmed device
//   LED_TYPE_STRIP    = WS2812 / NeoPixel-like strip on a single GPIO
typedef enum {
    LED_TYPE_OF = 0,
    LED_TYPE_STRIP = 1,
} LED_TYPE_t;

// ==== Per-output configuration ====
// For LED_TYPE_STRIP:
//   - led_count     = number of pixels on the strip
//   - gpio_or_addr  = GPIO number driving the strip (RMT/PWM/etc.)
//   - pca_channel   = ignored (set to 0)
//
// For LED_TYPE_OF (single/PCA channel):
//   - led_count     = 1
//   - gpio_or_addr  = if PCA-driven (designed), this is the 7-bit I2C addr; if GPIO-driven, this is the GPIO
//   - pca_channel   = 0..4 (PCA9955B channel index), or ignored for GPIO LEDs
typedef struct {
    LED_TYPE_t type;
    uint8_t led_count;
    uint8_t gpio_or_addr;
    uint8_t pca_channel;
} led_config_t;

#ifdef __cplusplus
}
#endif