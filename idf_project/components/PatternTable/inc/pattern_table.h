

#pragma once

#include "app_config.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "sdmmc_cmd.h"
#include "led_def.h"


// frame data contains a bool fade to record the fade
// a 2d array in uint8_t to save RGBA for every led


typedef struct {
    bool fade; //
    uint8_t colors[MAX_COLOR_VALUES][4]; // RGBA
} FrameData;


// PatternTable contains things about the whole patterntable and information to call every framedata

typedef struct {
    FILE *data_fp;// to record the open file frame data
    FILE *time_fp;// to record the open file time data

    int total_parts; // total num of ofs and leds in this patterntable
    int part_lengths[MAX_PARTS];// led num for each ofs and leds
    int total_leds;// total led nums we have(the sum of each led stripes and ofs(count as 1))
    int fps;//fps for changing frames
    int total_frames;// how many frames in pattern table
    int index; //current Frame in read from data file to esp32
    uint32_t frame_times[MAX_FRAMES];       // in milliseconds, to get timestamp for each frame from time file
    int frame_offsets[MAX_FRAMES];       // byte offset of each frame, where to jump if needed to read in mid 

    const char *mount_point;// to record mountpoint
    sdmmc_card_t *card;// to record card that need to mount a SDcard

    led_config_t led_config_arr[LED_CONFIG_NUM];// to record led_config_t of each ofs and leds
    
} PatternTable;


// init variable in Patterntable
void PatternTable_init(PatternTable *self, const char *mount_point);

// load time data from time data.txt to esp32 (change the file name in app_config.h)

esp_err_t PatternTable_load_times(PatternTable *self);

// load frame data from time data.txt to esp32 (change the file name in app_config.h)
// record how many leds and ofs, led num of each ledstripes
// construct led_config_arr
// here skip content of frame data, just record offset of the start of each frame
esp_err_t PatternTable_index_frames(PatternTable *self);

// jump to the frame we want(index , start from 0) and read that frame from frame data.txt
esp_err_t PatternTable_read_frame_at( PatternTable *self, const int index,FrameData *framedata);

// read a frame next(depend on index in pattern table) and read that frame from frame data.txt
esp_err_t PatternTable_read_frame_go_through( PatternTable *self,FrameData *framedata);

// construct led_config table for each frame
void PatternTable_construct_led_config(PatternTable *self);

//return timestamp array
const uint32_t *PatternTable_get_time_array(const PatternTable *self);

//return total_frames nums
int PatternTable_get_total_frames(const PatternTable *self);

//return total nums of leds and ofs(count as 1)
int PatternTable_get_total_leds(const PatternTable *self);

#ifdef __cplusplus
}
#endif