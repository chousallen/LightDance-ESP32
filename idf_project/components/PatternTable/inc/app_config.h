#pragma once
#include <stdint.h>

// --- timing / player in player.c---
#define DEFAULT_FPS               40           // default fps in frame play 
#define TIMER_RESOLUTION_HZ       1000000ULL   // 1 tick = 1 us, timer setting for clock alarm in calling frame play

// --- ledconfig default data in patterntable.c --- 
#define LED_CONFIG_NUM 30
#define LED_RGBS_BUF_LEN          1024 // buffer size for reading rgba, multiple of 512 better 
#define LED_DISCARD_BUF_LEN       512  // buffer size for discarding rgba when finding offsets, multiple of 512 better 
#define PATH_BUF_LEN              128  // string buffer for file name 

// --- SD  in sdcard.c---
#define SD_SPI_MAX_FREQ_KHZ       20000
#define SD_MAX_FILES              5
#define SD_ALLOC_UNIT_SIZE        (16 * 1024)
#define SD_MAX_TRANSFER_SIZE      (8 * 1024)


// --- patterntable default max num in patterntable.c---
#define MAX_PARTS 40 // MAX_PARTS : MAX total num of led stripes + OFs
#define MAX_COLOR_VALUES 4096  // MAX_PARTS : total num of leds 
#define MAX_FRAMES 2048 // MAX_PARTS : MAX total num of Frames


// --- MAIN in main.c---
#define MOUNT_POINT               "/sdcard"
#define TIME_DATA                 "800time.txt" //time data .txt name providing for patterntable
#define FRAME_DATA                "h800data.txt" // frame data.txt name providing for patterntable

