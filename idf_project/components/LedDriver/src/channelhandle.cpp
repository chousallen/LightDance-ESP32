#include "channelhandle.hpp"

static const char* TAG = "channelhandle";

esp_err_t ChannelHandle::config(const led_config_t config) {
    // Validate type first
    if(config.type != LED_TYPE_STRIP && config.type != LED_TYPE_OF) {
        ESP_LOGE(TAG, "invalid type=%d", (int)config.type);
        return ESP_ERR_INVALID_ARG;
    }
    type = config.type;

    if(type == LED_TYPE_STRIP) {
        esp_err_t ret = ws2812.config(config);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "ws2812 config failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    if(type == LED_TYPE_OF) {
        esp_err_t ret = pca9955.config(config);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "pca9955 config failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Should not reach here
    return ESP_ERR_INVALID_STATE;
}

esp_err_t ChannelHandle::write(const color_t* colors) {
    if(type != LED_TYPE_STRIP && type != LED_TYPE_OF) {
        ESP_LOGE(TAG, "write before config");
        return ESP_ERR_INVALID_STATE;
    }
    if(!colors) {
        ESP_LOGE(TAG, "colors is null");
        return ESP_ERR_INVALID_ARG;
    }

    if(type == LED_TYPE_STRIP) {
        esp_err_t ret = ws2812.write(colors);
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "ws2812 write failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // LED_TYPE_OF
    esp_err_t ret = pca9955.write(colors);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "pca9955 write failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t ChannelHandle::reconfigure(){
    esp_err_t ret = pca9955.reconfigure_marked();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "pca9955 reconfig failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t ChannelHandle::detach() {
    if(type != LED_TYPE_STRIP && type != LED_TYPE_OF) {
        // detach on unknown state; treat as already detached for idempotence
        return ESP_ERR_INVALID_STATE;
    }

    if(type == LED_TYPE_STRIP) {
        esp_err_t ret = ws2812.detach();
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "ws2812 detach failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // LED_TYPE_OF
    esp_err_t ret = pca9955.detach();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "pca9955 detach failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t ChannelHandle::wait_done() {
    if(type != LED_TYPE_STRIP && type != LED_TYPE_OF) {
        ESP_LOGE(TAG, "wait_done before config");
        return ESP_ERR_INVALID_STATE;
    }

    if(type == LED_TYPE_STRIP) {
        esp_err_t ret = ws2812.wait_done();
        if(ret != ESP_OK) {
            ESP_LOGE(TAG, "ws2812 wait_done failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // LED_TYPE_OF
    esp_err_t ret = pca9955.wait_done();
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "pca9955 wait_done failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

bool ChannelHandle::get_need_reconfig() {
    return pca9955.get_need_reconfig();
}