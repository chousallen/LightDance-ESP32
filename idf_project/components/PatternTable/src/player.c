#include "player_content.h"
#include "app_config.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "sdcard.h"          // for un/mount helpers
#include "esp_log.h"
#include "testing.h"
#include "player.h"

#include <stdio.h>           // fgets, printf
#include <string.h>          // strtok, strcmp, strcspn
#include <stdlib.h>          // atoi

#define TAG "state"


void player_init(player *p){

    //init variable
    ESP_ERROR_CHECK(player_reader_init(p,MOUNT_POINT,TIME_DATA,FRAME_DATA));
    player_var_init(p);
    ESP_LOGI(TAG,"start init");

    //2. get frame data ready to play
    ESP_ERROR_CHECK(PatternTable_read_frame_at(&p->pattern_table,p->reader_index,&p->fd_test[p->reader_index%2]));
    ESP_ERROR_CHECK(PatternTable_read_frame_go_through(&p->pattern_table,&p->fd_test[(p->reader_index+1)%2]));
    
    
    timer_init(p);
}

void player_start(player *p){

    if(p->state == STATE_DELAY||p->state == STATE_STOPPED ||p->state == STATE_IDLE){
        p->state = STATE_RUNNING;
        // p->reader_index = start_frame_index;
        player_start_content(p);
    }
    else{
        ESP_LOGW(TAG,"wrong state not allow to start");
        PlayerState stage_logout_ = player_get_state(p);
    }

}


void player_pause(player *p){

    if(p->state == STATE_RUNNING){
        player_pause_content(p);
        p->state = STATE_PAUSED;

    }
    else{
        ESP_LOGW(TAG,"wrong state not allow to paused");
        PlayerState stage_logout_ = player_get_state(p);
    }

}


void player_resume(player *p){
    
    if(p->state == STATE_PAUSED){
        player_resume_content(p);
        p->state = STATE_RUNNING;

    }
    else{
        ESP_LOGW(TAG,"wrong state not allow to resume");
        PlayerState stage_logout_ = player_get_state(p);
    }
}

void player_stop(player *p){

    if(p->state == STATE_RUNNING){

        p->state = STATE_STOPPED;
        player_pause_content(p);
        player_stop_content(p);

    }
    else if(p->state == STATE_PAUSED){
        p->state = STATE_STOPPED;
        player_stop_content(p);
    }
    else{
        ESP_LOGW(TAG,"wrong state not allow to stop");
        PlayerState stage_logout_ = player_get_state(p);
    }
}


void player_exit(player *p){
    
    
    if(p->state == STATE_STOPPED){

        p->state = STATE_EXITING;
        fclose(p->pattern_table.data_fp);
        unmount_sdcard(&p->pattern_table.card);
        ESP_LOGI(TAG, "Main exits.");

    }
    else{
        ESP_LOGW(TAG,"wrong state not allow to exit");
        PlayerState stage_logout_ = player_get_state(p);
    }
}
    
void player_delay(player *p,int delaytime,int delaylight){

    if(p->state == STATE_STOPPED ||p->state == STATE_IDLE){
        delaytime = 0;
        delaylight = 0;
        int64_t delay = perf_timer_start();
        while(!perf_timer_cnt(delay,delaytime,"DELAY", "DELAYTIME")){
            
            if(perf_timer_cnt(delay,delaylight,"DELAY", "DELAYlight")){

            }
            vTaskDelay(10);
        }
    
    }
    else{
        ESP_LOGI(TAG,"wrong state not allow to delay");
        PlayerState stage_logout_ = player_get_state(p);
     
    }
    
        
}

PlayerState player_get_state( player *p){
    
    if(p->state == STATE_IDLE ){
        ESP_LOGI(TAG,"now is STATE_IDLE ");
    }
    else if(p->state == STATE_RUNNING ){
        ESP_LOGI(TAG,"now is STATE_RUNNING ");
    }
    else if(p->state == STATE_PAUSED ){
        ESP_LOGI(TAG,"now is STATE_PAUSED ");
    }
    else if(p->state == STATE_STOPPED ){
        ESP_LOGI(TAG,"now is STATE_STOPPED ");
    }
    else if(p->state == STATE_EXITING ){
        ESP_LOGI(TAG,"now is STATE_EXITING ");
    }
    else if(p->state == STATE_DELAY ){
        ESP_LOGI(TAG,"now is STATE_DELAY ");
    }
    else{
        ESP_LOGI(TAG,"unknown stage ");
    }
    return p->state;
}