#include"app_config.h"
#include "pattern_table.h"
#include "player_content.h"
#include "sdcard.h"
#include "player.h"

#include <freertos/FreeRTOS.h>

#include "LedDriver.hpp"
#include "led_def.h"



#define TAG "PLAYER_MAIN"

static player P;

extern "C" void app_main(void);

#define N_STRIP_CH 4
#define N_OF_CH 10

void config_led_driver(LedDriver& ledDriver)
{
    led_config_t ch_configs[N_STRIP_CH + N_OF_CH];

    // PCA9955B addresses 0x1f, 0x20, 0x22, 0x23, 0x5b, 0x5c, 0x5e, 0x5f
    uint8_t pca_addresses[N_OF_CH/5] = {0x1f, 0x20}; 

    // Configure 4 LED strips
    for(int i=0; i<N_STRIP_CH; i++)
    {
        ch_configs[i].type = LED_TYPE_STRIP;
        ch_configs[i].led_count = 100;
        ch_configs[i].pca_channel = 0;
    }
    ch_configs[0].gpio_or_addr = 12;
    ch_configs[1].gpio_or_addr = 14;
    ch_configs[2].gpio_or_addr = 27;
    ch_configs[3].gpio_or_addr = 26;

    // Configure 20 optical fiber channels
    for(int i=0; i<N_OF_CH; i++)
    {
        ch_configs[N_STRIP_CH + i].type = LED_TYPE_OF;
        ch_configs[N_STRIP_CH + i].led_count = 1;
        ch_configs[N_STRIP_CH + i].gpio_or_addr = pca_addresses[i / 5]; // Distribute across 8 PCA chips
        ch_configs[N_STRIP_CH + i].pca_channel = i % 5; // Channel within each PCA
    }

    ledDriver.config(ch_configs, N_STRIP_CH + N_STRIP_CH);
}

color_t GREEN = {255, 0, 0};
color_t RED = {0, 255, 0};
color_t BLUE = {0, 0, 255};

color_t strip_frame[N_STRIP_CH][100];
color_t of_frame[N_OF_CH][1];
const color_t *cplt_frame[N_STRIP_CH + N_OF_CH];

void app_main(void)
{
    LedDriver ledDriver = LedDriver();
    config_led_driver(ledDriver);

    // Initialize complete frame pointers
    for (int i = 0; i < N_STRIP_CH; i++) {
        cplt_frame[i] = strip_frame[i];
    }
    for (int i = 0; i < N_OF_CH; i++) {
        cplt_frame[N_STRIP_CH + i] = of_frame[i];
    }

    while(1)
    {
        // Set strips to RED
        for(int i=0; i<N_STRIP_CH; i++)
        {
            for(int j=0; j<100; j++)
            {
                strip_frame[i][j] = RED;
            }
        }

        // Set optical fibers to RED
        for(int i=0; i<N_OF_CH; i++)
        {
            of_frame[i][0] = RED;
        }

        ledDriver.write(cplt_frame);

        ESP_LOGI("APP", "Set to RED");

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Set strips to GREEN
        for(int i=0; i<N_STRIP_CH; i++)
        {
            for(int j=0; j<100; j++)
            {
                strip_frame[i][j] = GREEN;
            }
        }

        // Set optical fibers to GREEN
        for(int i=0; i<N_OF_CH; i++)
        {
            of_frame[i][0] = GREEN;
        }

        ledDriver.write(cplt_frame);

        ESP_LOGI("APP", "Set to GREEN");

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Set strips to BLUE
        for(int i=0; i<N_STRIP_CH; i++)
        {
            for(int j=0; j<100; j++)
            {
                strip_frame[i][j] = BLUE;
            }
        }

        // Set optical fibers to BLUE
        for(int i=0; i<N_OF_CH; i++)
        {
            of_frame[i][0] = BLUE;
        }

        ledDriver.write(cplt_frame);

        ESP_LOGI("APP", "Set to BLUE");

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
        
  
    
    // player_init(&P);
    
    // player_start(&P);
    // vTaskDelay(500);
    // player_pause(&P);
    // player_pause(&P);
    // vTaskDelay(500);
    // player_resume(&P);
    // vTaskDelay(1000);
    // player_stop(&P);
    // vTaskDelay(300);
    // player_exit(&P);
}
