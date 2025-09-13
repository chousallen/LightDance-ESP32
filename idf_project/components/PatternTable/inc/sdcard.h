#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"


// cover all the things you need to do to mount a sdcard
// include spi_bus_config_t, spi_bus_initialize, sdmmc_host_t,
// esp_vfs_fat_sdmmc_mount_config_t, esp_vfs_fat_sdspi_mount.
esp_err_t mount_sdcard(sdmmc_card_t **g_card);


// esp_vfs_fat_sdcard_unmount and free spi bus
void unmount_sdcard(sdmmc_card_t **g_card);

#ifdef __cplusplus
}
#endif