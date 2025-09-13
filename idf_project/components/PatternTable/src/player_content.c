#include "app_config.h"
#include "esp_err.h"
#include "player_content.h"
#include "sdcard.h"
#include <stdio.h>                 // for fflush()
#include "esp_rom_sys.h"
#include "testing.h"
#include <stdio.h>           // fgets, printf
#include <string.h>          // strtok, strcmp, strcspn
#include <stdlib.h>          // atoi


#define TAG "player"

esp_err_t player_reader_init(player *p, const char *mount_point,const char *time_data, const char *frame_data ){
    p-> state = STATE_IDLE;
    p-> cnt = 0;
    p->  reader_index = 0;


    p->  suspend_detect_refresh = false;
 
    p-> s_refresh_task = NULL;

    p-> gptimer = NULL;
    
    // 1. mount SD
    if (mount_sdcard(&p->pattern_table.card) != ESP_OK) {
        ESP_LOGE("SD", "SD mount failed. Abort.");
        vTaskDelay(10);
        return ESP_FAIL;
    }

    //2. init pattern table and load time and data
    PatternTable_init(&p->pattern_table, mount_point);


    if (PatternTable_load_times(&p->pattern_table)!= ESP_OK) {
        ESP_LOGE("Player init", "Failed to load times.txt");
        unmount_sdcard(&p->pattern_table.card);
        vTaskDelay(10);
        return ESP_FAIL;
    }
    if (PatternTable_index_frames(&p->pattern_table)!= ESP_OK) {
        ESP_LOGE("Player init", "Failed to index data.txt");
        unmount_sdcard(&p->pattern_table.card);
        vTaskDelay(10);
        return ESP_FAIL;
    }

    ESP_LOGD("Player init", "FPS %d",p->pattern_table.fps);
    if(p->pattern_table.fps==0) p->pattern_table.fps = DEFAULT_FPS;

    ESP_LOGD("Player init", "FPS %d",p->pattern_table.fps);


    ESP_LOGI("Player init", "Total frames=%d, total_leds=%d, fps=%d",
             PatternTable_get_total_frames(&p->pattern_table),
             PatternTable_get_total_leds(&p->pattern_table),
             p->pattern_table.fps);
    fflush(stdout);

    return ESP_OK;
}


void player_var_init(player *p){
    
    p->  cnt = 0;

    p->  reader_index = 0;

    p->  suspend_detect_refresh = false;

    p-> s_refresh_task = NULL;

    p-> gptimer = NULL;

}


bool IRAM_ATTR example_timer_on_alarm_cb_v1(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *self)
{
    player *p = (player *)self;    
    p->cnt++;// count for changing frame
    
    // if refresh task is no in suspend, resume it 
    if(p->suspend_detect_refresh){
        p->suspend_detect_refresh = false;
        xTaskResumeFromISR(p->s_refresh_task );
        
        return true;
    }
    
    // if not in refresh task is no in suspend
    return false;
}


void refresh_task(void *arg){
    player *p = (player *)arg;    
    p->suspend_detect_refresh = true;
    vTaskSuspend(NULL);
    while(1){
        
        //refresh frame playing
        //reserved for led light 
        
        FrameData* tmp_framedata = player_get_current_play_framedata(p);
        led_config_t* tmp_led_config_t = player_get_LED_config_table(p);

        //
        //detect whether to change frame
        if ((p->cnt+1) *(1000/p->pattern_table.fps) >= p->pattern_table.frame_times[p->reader_index] ){
            
            //1. change index
            ESP_LOGD("refill","change frame");
            ESP_LOGD("IRAM", "%" PRIu32 "",p->pattern_table.frame_times[p->reader_index]);
            p->reader_index++;

            //2. load new frame 
            if (p->reader_index+1 < PatternTable_get_total_frames(&p->pattern_table)){
                ESP_LOGD("refill","refill");
                
                ESP_ERROR_CHECK(PatternTable_read_frame_go_through(&p->pattern_table,&p->fd_test[(p->reader_index-1)%2]));
    
            }
            ESP_LOGD("refill","READ finish");
            p->suspend_detect_refresh = true;
            vTaskSuspend(NULL);
        }
        
    }
}


//init timer for alarm
void timer_init(player *p){
    
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, 
        .direction = GPTIMER_COUNT_UP,      
        .resolution_hz = TIMER_RESOLUTION_HZ,   
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &p->gptimer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb_v1,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(p->gptimer, &cbs,p));
    ESP_LOGD("TIMER", "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(p->gptimer));

    
    gptimer_alarm_config_t alarm_config2 = {
        .reload_count = 0,
        .alarm_count = TIMER_RESOLUTION_HZ/p->pattern_table.fps, // period = 1s
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(p->gptimer, &alarm_config2));
    ESP_LOGD("TIMER", "Start timer, auto-reload at alarm event");
}


void player_start_content(player *p){
    ESP_ERROR_CHECK(gptimer_start(p->gptimer));
    ESP_LOGI("PLAYER", "Start");
    xTaskCreate(refresh_task, "refresh_task", 4096, p, 5, &p->s_refresh_task );
}

void player_pause_content(player *p){
    ESP_ERROR_CHECK(gptimer_stop(p->gptimer));
    ESP_ERROR_CHECK(gptimer_disable(p->gptimer));
  
    ESP_LOGI("PLAYER", "pause");
}

void player_resume_content(player *p){
    ESP_ERROR_CHECK(gptimer_enable(p->gptimer));
    ESP_ERROR_CHECK(gptimer_start(p->gptimer));
    ESP_LOGI("PLAYER", "resume");
}


void player_stop_content(player *p){

    ESP_LOGI("PLAYER", "stop");
    ESP_ERROR_CHECK(gptimer_del_timer(p->gptimer));
    ESP_LOGD("TIMER", "delete");
    ESP_LOGD("FILE", "delete");
    vTaskDelete(p->s_refresh_task);
    ESP_LOGD("TASK", "delete");
}

const FrameData* player_get_current_play_framedata(const player*p){
    return &p->fd_test[p->reader_index%2];
}

const led_config_t* player_get_LED_config_table(const player*p){
    return p->pattern_table.led_config_arr;
}