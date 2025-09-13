#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gptimer.h"     // for gptimer_handle_t
#include "pattern_table.h"      // for PatternTable, FrameData
#include "sdmmc_cmd.h"          // for sdmmc_card_t
#include "testing.h"
#include "led_def.h"


// use to control stage change for player
typedef enum {
    STATE_IDLE = 0,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_STOPPED,
    STATE_DELAY,
    STATE_EXITING
} PlayerState;



// contains things we need to play frames 
typedef struct {

    int cnt ;// timer counter , use to record how many times alarm has happend ,to know when to change frame
    int reader_index;//record which frame we read now

    bool suspend_detect_refresh ;// use to control refresh frame and change frame task 


    TaskHandle_t s_refresh_task ;

    PatternTable pattern_table;
    FrameData fd_test[2];// two frame , serve as buffer in frame reading
    gptimer_handle_t gptimer;// timer use in control alarm

    PlayerState state;

} player;


// init variants, also mount sdcard ,getting timedata.txt, getting framedata.txt
esp_err_t player_reader_init(player *p, const char *mount_point,const char *time_data, const char *frame_data);

//only init variants
void player_var_init(player *p);

// init timer for making alarm 
void timer_init(player *p);

// content of alarm, calling refresh task to work
bool example_timer_on_alarm_cb_v1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *self);

// refresh frame (calling led), and change frame if needed
void refresh_task(void *arg);

// start timer to call alarm and refresh task
void player_start_content(player *p);

// if in pause, start timer to make player resume
void player_resume_content(player *p);

// pause the player by pause the timer
void player_pause_content(player *p);

// stop the player, delete timer and refresh task
void player_stop_content(player *p);

//return current displaying frame
const FrameData* player_get_current_play_framedata(const player*p);

//return led config table
const led_config_t* player_get_LED_config_table(const player*p);


#ifdef __cplusplus
}
#endif