#pragma once

extern "C" {
#include "pca9955.h"
}

#define IREFALL_addr 0x45
#define OF_MAXIMUM_BRIGHTNESS 255

/**
 * @brief PCA9955B LED driver interface class.
 *
 * Provides methods to configure, control, and communicate with a PCA9955B
 * constant current LED driver via I2C.
 */
class pca9955Driver {
  public:
    /** @brief Construct a new PCA9955 driver instance. */
    pca9955Driver();

    /**
     * @brief Configure the PCA9955 driver instance with LED settings.
     * Initializes the driver with the specified PCA9955 channel and I2C address,
     * obtains the I2C bus handle, locates the device index, and increments its
     * usage counter.
     *
     * @param[in] config LED configuration parameters (channel, I2C address, etc.).
     *
     * @return
     *  - ESP_OK: Configuration successful.
     *  - Error codes from I2C bus operations if initialization fails.
     */
    esp_err_t config(const led_config_t config);

    /**
     * @brief Write RGB color data to the PCA9955 LED driver.
     *
     * Sends the RGB values for the selected PCA9955 channel over I2C,
     * starting from the corresponding PWM register address.
     *
     * @param[in] colors Pointer to color_t containing
     *                   the red, green, and blue values.
     *
     * @return
     *  - ESP_OK: Data written successfully.
     *  - Other error codes from I2C transmit function.
     */
    esp_err_t write(const color_t* colors);

    /**
     * @brief Reconfigure all PCA9955 devices marked as needing reinitialization.
     *
     * This function iterates through the internal reconfiguration list and attempts
     * to recover each PCA9955 device in-place:
     *  - Removes the old I2C device handle (best effort, does not abort on failure).
     *  - Re-adds the device on the same address without altering slot usage counts.
     *  - Performs chip-wide initialization (e.g., reset IREFALL).
     *  - Updates the shared device handle and clears the reconfiguration flag if successful.
     *
     * Devices that fail reconfiguration remain marked for retry on the next call.
     *
     * @return
     *  - ESP_OK if all marked devices were successfully reconfigured.
     *  - First error encountered if one or more devices failed to reconfigure.
     */
    esp_err_t reconfigure_marked();

    /**
     * @brief Detach the PCA9955 device from the I2C bus.
     *
     * Decrements the usage count for the current device index.
     * If the usage count reaches zero, the device is unregistered:
     * - `registered` count is decremented
     * - I2C device is removed via @ref i2c_master_bus_rm_device()
     * - Address and device handle entries are cleared
     *
     * @return
     *  - ESP_OK on success; ESP_ERR_INVALID_STATE if not attached; otherwise I2C errors.
     */
    esp_err_t detach();

    /**
     * @brief Wait until the PCA9955 finishes its current operation.
     *
     * Currently implemented as a stub that always returns ESP_OK.
     * Intended to be expanded if asynchronous operations are added.
     *
     * @return
     *  - ESP_OK: Always, in current implementation.
     */
    esp_err_t wait_done();

    /**
     * @brief Find or register a PCA9955 device on the I2C bus.
     *
     * Searches existing records for the given 7-bit I2C address. If found, returns
     * its index. If not found, creates a new I2C device with that address, stores
     * the handle and address, initializes IREFALL to default brightness, and
     * returns the new index.
     *
     * @param[in] addr 7-bit I2C slave address of the PCA9955.
     *
     * @return
     *  Index (0..MAX_PCA9955_NUM-1) on success; -1 on failure.
     */
    int get_or_register_device(uint8_t addr);

    /** 
     * @brief Get the  flag "need_reconfig" declared in pca9955driver.cpp.
     * 
     * @return whether reconfiguration is required.
    */
    bool get_need_reconfig();

  private:
    uint8_t addr;                       /**< I2C address of the device. */
    uint8_t pca_channel;                /**< PCA9955 channel index. */
    int idx;                            /**< Internal index for device tracking. */
    i2c_master_bus_handle_t bus_handle; /**< I2C bus handle. */
    i2c_master_dev_handle_t dev_handle; /**< Pointer to I2C device handle. */
    uint8_t cmd[4];                     /**< Scratch buffer for I2C Tx */
};