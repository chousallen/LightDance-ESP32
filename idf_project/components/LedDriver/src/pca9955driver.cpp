#include "pca9955driver.hpp"

static const char* TAG = "pca9955driver";
static uint8_t pca_addr[MAX_PCA9955_NUM];
static uint8_t use_count[MAX_PCA9955_NUM] = {0};
static bool reconfig_list[MAX_PCA9955_NUM] = {0};
static i2c_master_dev_handle_t dev_handles[MAX_PCA9955_NUM];
static int registered = 0;
static bool need_reconfig = false;

// 1xxxxxxx: auto increment
static const uint8_t PWM_addr[5] = {0x88, 0x8B, 0x8E, 0x91, 0x94};

pca9955Driver::pca9955Driver() {}

esp_err_t pca9955Driver::config(const led_config_t config) {
    if(config.pca_channel > 4) {  // we only have 5 base addresses in PWM_addr[]
        return ESP_ERR_INVALID_ARG;
    }

    pca_channel = config.pca_channel;
    addr = config.gpio_or_addr;

    ESP_RETURN_ON_ERROR(i2c_master_get_bus_handle(I2C_NUM_0, &bus_handle), TAG, "get_bus_handle failed");

    int found = get_or_register_device(addr);
    if(found < 0) {
        return ESP_FAIL;  // already logged inside find() if you add logs
    }

    use_count[found]++;
    dev_handle = dev_handles[found];
    idx = found;
    return ESP_OK;
}

esp_err_t pca9955Driver::write(const color_t* colors) {
    cmd[0] = PWM_addr[pca_channel];
    cmd[1] = colors[0].red;
    cmd[2] = colors[0].green;
    cmd[3] = colors[0].blue;
    esp_err_t ret = i2c_master_transmit(dev_handle, cmd, sizeof(uint8_t) * 4, I2C_TRANSMIT_TIMEOUT);
    if(ret != ESP_OK){
        // Mark this slot for reconfiguration and set the global flag.
        ESP_LOGE(TAG, "pca9955b address %d write failed: %s", addr, esp_err_to_name(ret));
        need_reconfig = true;
        reconfig_list[idx] = true;
    }
    return ret;       
}

esp_err_t pca9955Driver::reconfigure_marked() {
    if (!need_reconfig) return ESP_OK;  // quick exit

    esp_err_t first_err = ESP_OK;
    bool any_remaining = false;

    for (int i = 0; i < MAX_PCA9955_NUM; ++i) {
        if (!reconfig_list[i]) continue;  // only process marked slots

        uint8_t addr_i = pca_addr[i];
        i2c_master_dev_handle_t old = dev_handles[i];

        // (1) Best-effort: try to remove the old handle; DO NOT abort on failure.
        if (old) {
            esp_err_t rm_err = i2c_master_bus_rm_device(old);
            if (rm_err != ESP_OK) {
                ESP_LOGW(TAG, "rm_device(0x%02X) failed: %s; continue reconfig...",
                         addr_i, esp_err_to_name(rm_err));
            }
            dev_handles[i] = nullptr;
        }

        // (2) Re-add the device handle at the same address (in-place).
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = addr_i,
            .scl_speed_hz    = I2C_SCL_FREQ,  //100000, adjust in led_def.h
            .scl_wait_us     = 0,
            .flags           = {0},     // ensure ACK check is not disabled
        };

        i2c_master_dev_handle_t dev_new = nullptr;
        esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_new);
        if (err != ESP_OK || dev_new == nullptr) {
            if (first_err == ESP_OK) first_err = (err == ESP_OK ? ESP_FAIL : err);
            any_remaining = true;                 // keep the mark for future retry
            ESP_LOGE(TAG, "add_device 0x%02X failed: %s",
                     addr_i, esp_err_to_name(err));
            continue;
        }

        // (3) Chip-scope init (match get_or_register_device()).
        //     If this fails, drop the new handle and keep the mark.
        uint8_t init_cmd[2] = { IREFALL_addr, (uint8_t)OF_MAXIMUM_BRIGHTNESS };
        err = i2c_master_transmit(dev_new, init_cmd, sizeof(init_cmd), I2C_TRANSMIT_TIMEOUT);
        if (err != ESP_OK) {
            (void)i2c_master_bus_rm_device(dev_new);
            if (first_err == ESP_OK) first_err = err;
            any_remaining = true;
            ESP_LOGE(TAG, "init IREFALL 0x%02X failed: %s",
                     addr_i, esp_err_to_name(err));
            continue;
        }

        // (4) Success: publish the new handle, clear the mark.
        dev_handles[i] = dev_new;
        reconfig_list[i] = false;

        // Sync this instance if it is bound to this slot
        if (idx == i) dev_handle = dev_new;

        ESP_LOGI(TAG, "Reconfig OK at addr 0x%02X (slot %d)", addr_i, i);
    }

    // Update the global flag based on remaining marks
    for (int i = 0; i < MAX_PCA9955_NUM; ++i) {
        if (reconfig_list[i]) { any_remaining = true; break; }
    }
    need_reconfig = any_remaining;

    return first_err;  // ESP_OK if all succeeded; otherwise first failure
}

esp_err_t pca9955Driver::detach() {
    if(idx < 0) {
        return ESP_ERR_INVALID_STATE;
    }

    if(--use_count[idx] == 0) {
        registered--;
        esp_err_t rm_err = i2c_master_bus_rm_device(dev_handles[idx]);
        // clear local registry slot regardless of rm_err, but still return error if any
        pca_addr[idx] = 0;
        dev_handles[idx] = NULL;
        idx = -1;
        dev_handle = nullptr;
        return rm_err;
    }

    idx = -1;
    dev_handle = nullptr;
    return ESP_OK;
}

esp_err_t pca9955Driver::wait_done() {
    return ESP_OK;
}

int pca9955Driver::get_or_register_device(uint8_t addr) {
    // 1) Already registered?
    for(int i = 0; i < MAX_PCA9955_NUM; i++) {
        if(pca_addr[i] == addr) {
            return i;
        }
    }

    // 2) Capacity check
    if(registered >= MAX_PCA9955_NUM) {
        return -1;
    }

    // 3) Add new device on the bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_SCL_FREQ,  // 100000, adjust in led_def.h
        .scl_wait_us = 0,        // add: silence -Wmissing-field-initializers
        .flags = {0},            // add: zero all flag bits
    };

    i2c_master_dev_handle_t dev = NULL;
    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev);
    if(err != ESP_OK || dev == NULL) {
        return -1;
    }

    // 4) Initialize device (set IREFALL)
    uint8_t local_cmd[2] = {IREFALL_addr, (uint8_t)OF_MAXIMUM_BRIGHTNESS};
    err = i2c_master_transmit(dev, local_cmd, sizeof(local_cmd), I2C_TRANSMIT_TIMEOUT);
    if(err != ESP_OK) {
        (void)i2c_master_bus_rm_device(dev);  // best-effort cleanup
        return -1;
    }

    // 5) Commit to tables only after success
    int index = registered;
    pca_addr[index] = addr;
    dev_handles[index] = dev;
    registered++;
    return index;
}

bool pca9955Driver::get_need_reconfig(){
    return need_reconfig;
}