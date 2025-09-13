#include"app_config.h"
#include "esp_err.h"
#include "sdcard.h"
#define TAG "SDcard"

// ==== SDSPI PIN definition ====
#define PIN_NUM_MISO  2
#define PIN_NUM_MOSI  15
#define PIN_NUM_CLK   14
#define PIN_NUM_CS    13
#define SPI_DMA_CHAN   1

esp_err_t mount_sdcard(sdmmc_card_t **g_card) {
    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SD_MAX_TRANSFER_SIZE,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CHAN));

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SD_SPI_MAX_FREQ_KHZ ;  //not sure
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_MAX_FILES,
        .allocation_unit_size = SD_ALLOC_UNIT_SIZE
    };

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, g_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return ESP_FAIL;
    }

    sdmmc_card_print_info(stdout, *g_card);
    return ESP_OK;
}

void unmount_sdcard(sdmmc_card_t **g_card) {
    if (*g_card) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, *g_card);
        *g_card = NULL;
    }
    spi_bus_free(SPI2_HOST);
}