#ifndef ENGINE_TIME_H
#define ENGINE_TIME_H

#include <time.h>
#include <engine/types.h>

float time_delta;
double time_elapsed_time;

void time_update_delta();

void time_start();

float time_end();

extern float render_frame_time;
extern float disired_frame_time;

extern u8 frames;
extern float frame_second;

static struct timespec time_diffence(struct timespec start, struct timespec end)
{
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}
#endif
