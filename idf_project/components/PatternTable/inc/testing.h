#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


// to start timer，use a int64_t to record the current time and return to serve ad starting time
int64_t perf_timer_start(void);

// give the timer (start_time), output tag and message also how long it pass 
// (mainly for testing how long a funcion take)
void perf_timer_end(int64_t start_time, const char *tag, const char *msg);

// give the timer (start_time) and how long you want to wait(end_time) in "us" 
// if times up,output tag and message also how long it pass ,return true
// if not yet ,return false
bool perf_timer_cnt(int64_t start_time,int64_t end_time, const char *tag, const char *msg);

#ifdef __cplusplus
}
#endif