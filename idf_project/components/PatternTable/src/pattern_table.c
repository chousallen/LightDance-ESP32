#include "app_config.h"
#include "esp_err.h"
#include "pattern_table.h"
#include "sdcard.h"
#include "esp_log.h"
#include <stdio.h>      
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "testing.h"
#include "led_def.h"

#define TAG "LD_READER"



// default for led_config_t for every led
// LED_CONFIG_NUM could be change in app_config
// if change dont forget to fix the default table here
const uint8_t GPIO_OR_ADDR[LED_CONFIG_NUM] = {
    
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    
};

const uint8_t PCA_CHANNEL[LED_CONFIG_NUM] = {
    
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    0,  0,  0,  0,  0, 
    0,  0,  0,  0,  0,  
    
};

//init variable
void PatternTable_init(PatternTable *self, const char *mount_point) {
    self->mount_point = mount_point;
    self->data_fp = NULL;
    self->time_fp = NULL;
    self->total_parts = 0;
    self->fps = 0;
    self->total_frames = 0;
    self->total_leds = 0;
    self->index = 0;
}


esp_err_t PatternTable_load_times(PatternTable *self) {

    // 1. open time.txt
    char path[PATH_BUF_LEN];
    snprintf(path, sizeof(path), "%s/%s", self->mount_point, TIME_DATA);
    self->time_fp = fopen(path, "r");
    if (!self->time_fp) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        vTaskDelay(10);
        return ESP_FAIL;
    }

    // 2.load content and count total frames
    self->total_frames = 0;
    while (fscanf(self->time_fp, "%lu", &self->frame_times[self->total_frames]) == 1) {
        self->total_frames++;
        if (self->total_frames >= MAX_FRAMES) break;
    }

    fclose(self->time_fp);
    ESP_LOGI(TAG, "Loaded %d frame times", self->total_frames);
    return ESP_OK;
}

// turning hex into uint8_t
static inline uint8_t hex_nibble(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    c = (char)tolower((unsigned char)c);
    return (uint8_t)(10 + (c - 'a'));
}

//combine hexpair to make byte information
static inline uint8_t hexpair_to_byte(char hi, char lo) {
    return (uint8_t)((hex_nibble(hi) << 4) | hex_nibble(lo));
}

// throw away line end（support \n or \r\n）
static void eat_eol(FILE *fp) {
    int c = fgetc(fp);
    if (c == '\r') {
        int c2 = fgetc(fp);
        if (c2 != '\n' && c2 != EOF) ungetc(c2, fp);
    } else if (c != '\n' && c != EOF) {
        ungetc(c, fp);
    }
}

// drop need chars with small buffer（to skip dense hex line while building index array）
static esp_err_t discard_exact_chars(FILE *fp, size_t need) {
    char buf[LED_DISCARD_BUF_LEN];
    while (need > 0) {
        size_t chunk = need < sizeof(buf) ? need : sizeof(buf);
        size_t got = fread(buf, 1, chunk, fp);
        if (got != chunk){
            ESP_LOGE(TAG,"didn't get expected chars to discard");
            return ESP_FAIL;
        } 
        need -= got;
    }
    eat_eol(fp);
    return ESP_OK;
}


/**
 * 
 * - chars we expected = need_hex
 * - after reading data we drop line end（\n or \r\n）
 * - return false data isn't enough
 *
 * attention : we assume that data only composed of 0-9A-Fa-f
 * 
 */
static esp_err_t stream_dense_hex_to_bytes(FILE *fp, size_t need_hex, uint8_t *out) {
    // 2 hex → 1 byte
    const size_t need_bytes = need_hex / 2;
    const size_t BUF_SZ = LED_RGBS_BUF_LEN; // file read buffer
    char buf[BUF_SZ];
    size_t produced = 0;  //  bytes already created
    int have_hi = 0;      // whether higher half exits
    char hi = 0;

    while (produced < need_bytes) {
        size_t remain_hex = (need_bytes - produced) * 2 + (have_hi ? 1 : 0);
        size_t to_read = remain_hex < BUF_SZ ? remain_hex : BUF_SZ;

        size_t got = fread(buf, 1, to_read, fp);
        if (got != to_read){
            ESP_LOGE(TAG,"didn't get expected data when turning hex to bytes");
            return ESP_FAIL; // dense pattern : we expected to read data precisely

        } 
        for (size_t i = 0; i < got; ++i) {
            char c = buf[i];
            
            if (c == '\r' || c == '\n') return false;

            if (!have_hi) {
                hi = c;
                have_hi = 1;
            } else {
                out[produced++] = hexpair_to_byte(hi, c);
                have_hi = 0;
            }
        }
    }

    
    eat_eol(fp);
    return ESP_OK;
}



esp_err_t PatternTable_index_frames(PatternTable *self) {

    //1. open data.txt
    char path[PATH_BUF_LEN];
    snprintf(path, sizeof(path), "%s/%s", MOUNT_POINT, FRAME_DATA);
    self->data_fp = fopen(path, "r");
    if (!self->data_fp) {
        ESP_LOGE(TAG, "Failed to open %s", path);
        return ESP_FAIL;
    }

    // 2. read total num of leds and ofs
    if (fscanf(self->data_fp, "%d", &self->total_parts) != 1){
        ESP_LOGE(TAG, "Failed to scan data file total parts");
        return ESP_FAIL;
    } 

    // 3. read num of ledstripes and sum for total leds
    self->total_leds = 0;
    for (int i = 0; i < self->total_parts; ++i) {
        if (fscanf(self->data_fp, "%d", &self->part_lengths[i]) != 1){
            ESP_LOGE(TAG, "Failed to scan data file part length [%d]",i);
            return ESP_FAIL;
        } 
        self->total_leds += self->part_lengths[i];
    }

    PatternTable_construct_led_config(self);


    // 4. load fps
    if (fscanf(self->data_fp, "%d", &self->fps) != 1){
        ESP_LOGE(TAG, "Failed to scan data file fps");
        return ESP_FAIL;
    } 
    ESP_LOGD(TAG, "FPS %d", self->fps);
  
    int c;
    while ((c = fgetc(self->data_fp)) != '\n' && c != EOF) {}

    // 5. frames index
    const size_t need_hex = (size_t)self->total_leds * 8; // RRGGBBAA per LED
    self->total_frames = 0;

    for (;;) {
        long offset = ftell(self->data_fp);
        if (offset < 0){
            ESP_LOGE(TAG,"minus offset");
            return ESP_FAIL;
        } 

        int fade_dummy;
        if (fscanf(self->data_fp, "%d", &fade_dummy) != 1){
            break;
        } 

        if (self->total_frames >= MAX_FRAMES){
            ESP_LOGE(TAG,"total frames are more than default MAX FRAMES");
            return ESP_FAIL;
        } 
        self->frame_offsets[self->total_frames] = (int)offset;
        ESP_LOGD("Ftell", "frame %d starts @ %ld", self->total_frames, offset);
        self->total_frames++;

        
        while ((c = fgetc(self->data_fp)) != '\n' && c != EOF) {}

        ESP_ERROR_CHECK(discard_exact_chars(self->data_fp, need_hex));
    }

    ESP_LOGI(TAG, "Indexed %d frames", self->total_frames);
    self->index = 0;
    return ESP_OK;
}


esp_err_t PatternTable_read_frame_at(PatternTable *self, const int index, FrameData *framedata) {

    // 1. check data file has been opened
    if (!self->data_fp ){
        ESP_LOGE(TAG,"no data file open");
        return ESP_FAIL;
    }    
    // 2. check index (default 0)
    if ( index < 0 || index >= self->total_frames){
        ESP_LOGE(TAG,"wrong index");
        return ESP_FAIL;
    }
    // 3. jump to that offset
    fseek(self->data_fp, self->frame_offsets[index], SEEK_SET);
    int64_t read_timer = perf_timer_start();

    //4. get fade
    int fade;
    if (fscanf(self->data_fp, "%d", &fade) != 1){
        ESP_LOGE(TAG,"failed to get fade");
        return ESP_FAIL;
    } 
    framedata->fade = (fade != 0);//making default as 0

   //5. get frame data RGBA
    int c;
    while ((c = fgetc(self->data_fp)) != '\n' && c != EOF) {}

   
    const size_t need_hex = (size_t)self->total_leds * 8;
    uint8_t *out = (uint8_t *)framedata->colors; // continuing 4 bytes/LED (RGBA)
    if (stream_dense_hex_to_bytes(self->data_fp, need_hex, out) != ESP_OK){
        ESP_LOGE(TAG,"failed to turn hex to byte in read frame at");
        return ESP_FAIL;
    } 

    perf_timer_end(read_timer, "frame input", "end");

    self->index = index + 1; // enable go_through() to read next index
    return ESP_OK;
}


esp_err_t PatternTable_read_frame_go_through(PatternTable *self, FrameData *framedata) {
    
    // 1. check data file has been opened
    if (!self->data_fp ){
        ESP_LOGE(TAG,"no data file open");
        return ESP_FAIL;
    }    

    // 2. check index(don't overflow)
    if ( self->index >= self->total_frames){
        ESP_LOGI(TAG,"already last frames");
        return ESP_OK;
    }

    //3. get fade
    int fade;
    if (fscanf(self->data_fp, "%d", &fade) != 1){
        ESP_LOGE(TAG,"failed to ge t fade");
        return ESP_FAIL;
    } 
    framedata->fade = (fade != 0);

    //4. get frame data RGBA
    int c;
    while ((c = fgetc(self->data_fp)) != '\n' && c != EOF) {}

    const size_t need_hex = (size_t)self->total_leds * 8;
    uint8_t *out = (uint8_t *)framedata->colors;
    if (stream_dense_hex_to_bytes(self->data_fp, need_hex, out) != ESP_OK){
        ESP_LOGE(TAG,"failed to turn hex to byte in read frame at");
        return ESP_FAIL;
    } 


    self->index++;
    return ESP_OK;
}




const uint32_t *PatternTable_get_time_array(const PatternTable *self) {
    return self->frame_times;
}

int PatternTable_get_total_frames(const PatternTable *self) {
    return self->total_frames;
}

int PatternTable_get_total_leds(const PatternTable *self) {
    return self->total_leds;
}

void PatternTable_construct_led_config(PatternTable *self){
    for(int i=0; i<self->total_parts;i++){

        self-> led_config_arr[i].led_count = self->part_lengths[i];
        if(self-> led_config_arr[i].led_count == 1){
            self-> led_config_arr[i].type = LED_TYPE_OF;
        } else{
            self-> led_config_arr[i].type = LED_TYPE_STRIP;
        }
        self-> led_config_arr[i].gpio_or_addr = GPIO_OR_ADDR[i];
        self-> led_config_arr[i].pca_channel = PCA_CHANNEL[i];
    }
}