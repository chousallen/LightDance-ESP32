#include "LedDriver.hpp"

static const char* TAG = "LedDriver";

// Only the first initializer is needed; the rest are zero-initialized by C.
static const color_t blank[127] = {{.green = 0, .red = 0, .blue = 0}};

ChannelHandle LedDriver::channel_handle[MAX_CHANNEL_NUM];

LedDriver::LedDriver(): ch_num(-1), bus_handle(nullptr) {
    esp_err_t err = i2c_bus_init(I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, &bus_handle);
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed: %s", esp_err_to_name(err));
        bus_handle = nullptr;  // mark unusable
    }
}

LedDriver::~LedDriver() {
    (void)clear_frame();  // best-effort
    (void)reset();        // best-effort
    if(bus_handle) {
        esp_err_t err = i2c_del_master_bus(bus_handle);
        if(err != ESP_OK) {
            ESP_LOGW(TAG, "i2c_del_master_bus failed: %s", esp_err_to_name(err));
        }
        bus_handle = nullptr;
    }
}

esp_err_t LedDriver::config(const led_config_t* configs, const int _ch_num) {
    if(configs == nullptr) {
        ESP_LOGE(TAG, "configs is null");
        return ESP_ERR_INVALID_ARG;
    }
    if(_ch_num < 0 || _ch_num > MAX_CHANNEL_NUM) {
        ESP_LOGE(TAG, "Invalid channel number: %d", _ch_num);
        return ESP_ERR_INVALID_ARG;
    }
    if(!bus_handle) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;  // bus not available
    }

    // If already configured, detach previous channels first.
    if(ch_num != -1) {
        esp_err_t r = reset();
        if(r != ESP_OK)
            return r;
    }

    // Configure each channel; rollback on failure.
    for(int i = 0; i < _ch_num; ++i) {
        esp_err_t r = channel_handle[i].config(configs[i]);
        if(r != ESP_OK) {
            ESP_LOGE(TAG, "channel %d config failed: %s", i, esp_err_to_name(r));
            // for(int j = 0; j < i; ++j) {
            //     (void)channel_handle[j].detach();
            // }
            // ch_num = -1;
            // return r;
        }
    }

    ch_num = _ch_num;

    // Clear to a known state (best-effort, but propagate error if any).
    esp_err_t clr = clear_frame();
    if(clr != ESP_OK) {
        ESP_LOGW(TAG, "clear_frame after config failed: %s", esp_err_to_name(clr));
        return clr;
    }
    return ESP_OK;
}

esp_err_t LedDriver::write(const color_t** frame) {
    if(ch_num < 0) {
        return ESP_ERR_INVALID_STATE;  // not configured
    }
    if(!frame) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t agg = ESP_OK;
    for(int i = 0; i < ch_num; ++i) {
        const color_t* buf = frame[i];
        if(!buf) {
            ESP_LOGE(TAG, "frame[%d] is null", i);
            agg = ESP_ERR_INVALID_ARG;
            continue;
        }
        esp_err_t r = channel_handle[i].write(buf);
        if(r != ESP_OK) {
            ESP_LOGE(TAG, "write ch %d failed: %s", i, esp_err_to_name(r));
            agg = r;  // keep last error
        }
    }

    //do reconfigure
    if(channel_handle[0].get_need_reconfig()){
        esp_err_t r = channel_handle[0].reconfigure();
        if(r != ESP_OK){
            ESP_LOGE(TAG, "reconfigure failed: %s", esp_err_to_name(r));
            agg = r;
        }
    }
    
    if(agg == ESP_OK)
        agg = wait_all_done();
    return agg;
}

esp_err_t LedDriver::part_test(const int ch_idx, const color_t* colors) {
    if(ch_num < 0)
        return ESP_ERR_INVALID_STATE;
    if(ch_idx < 0 || ch_idx >= ch_num)
        return ESP_ERR_INVALID_ARG;
    if(!colors)
        return ESP_ERR_INVALID_ARG;

    esp_err_t r = channel_handle[ch_idx].write(colors);
    if(r != ESP_OK)
        return r;
    return channel_handle[ch_idx].wait_done();
}

esp_err_t LedDriver::reset() {
    if(ch_num < 0) {
        return ESP_OK;  // already detached/not configured
    }
    esp_err_t agg = ESP_OK;
    for(int i = 0; i < ch_num; ++i) {
        esp_err_t r = channel_handle[i].detach();
        if(r != ESP_OK) {
            ESP_LOGW(TAG, "detach ch %d failed: %s", i, esp_err_to_name(r));
            agg = r;
        }
    }
    ch_num = -1;
    return agg;
}
esp_err_t LedDriver::clear_frame() {
    if(ch_num < 0)
        return ESP_OK;

    esp_err_t agg = ESP_OK;
    for(int i = 0; i < ch_num; ++i) {
        // By default we pass a single zero color. If a backend needs N zeros,
        // it should expand/repeat internally; otherwise use 'blank' as needed.
        const color_t* buf = blank;
        esp_err_t r = channel_handle[i].write(buf);
        if(r != ESP_OK) {
            // Fallback: try using the larger zero buffer (if the backend expects more).
            ESP_LOGW(TAG, "clear ch %d with single zero failed (%s), trying bulk", i, esp_err_to_name(r));
            r = channel_handle[i].write(blank);
        }
        if(r != ESP_OK) {
            ESP_LOGE(TAG, "clear ch %d failed: %s", i, esp_err_to_name(r));
            agg = r;
        }
    }
    return agg;
}

esp_err_t LedDriver::wait_all_done() {
    if(ch_num < 0)
        return ESP_OK;

    esp_err_t agg = ESP_OK;
    for(int i = 0; i < ch_num; ++i) {
        esp_err_t r = channel_handle[i].wait_done();
        if(r != ESP_OK) {
            ESP_LOGE(TAG, "wait ch %d failed: %s", i, esp_err_to_name(r));
            agg = r;
        }
    }
    return agg;
}