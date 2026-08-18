#include "time.h"
#include <stdio.h>

#include <time.h>
#include <unistd.h>

#define NSEC_PER_SEC 1000000000L
#define TARGET_FPS 30.0f
#define TARGET_FRAME_TIME_NSEC (NSEC_PER_SEC / TARGET_FPS)


typedef struct timespec PTime;
static PTime frame_start_time;
static PTime frame_start_time_input;
PTime delta_time_counter;

double delta_time;


void start_frame_timer(PTime time) {
    clock_gettime(CLOCK_MONOTONIC, &time);
}

static void delay_for_frame(PTime time) {
    PTime frame_end_time;
    clock_gettime(CLOCK_MONOTONIC, &frame_end_time);

    long elapsed_nsec = (frame_end_time.tv_sec - time.tv_sec) * NSEC_PER_SEC +
                       (frame_end_time.tv_nsec - time.tv_nsec);

    if (elapsed_nsec < TARGET_FRAME_TIME_NSEC) {
        PTime sleep_time;
        long remaining_nsec = TARGET_FRAME_TIME_NSEC - elapsed_nsec;

        sleep_time.tv_sec = remaining_nsec / NSEC_PER_SEC;
        sleep_time.tv_nsec = remaining_nsec % NSEC_PER_SEC;

        nanosleep(&sleep_time, NULL);
    }
}

void calculate_delta_time(PTime last_frame_time) {
  PTime current_time;
  clock_gettime(CLOCK_MONOTONIC, &current_time);
  delta_time =
      (double)(current_time.tv_sec - last_frame_time.tv_sec) +
      (double)(current_time.tv_nsec - last_frame_time.tv_nsec) / NSEC_PER_SEC;

  last_frame_time = current_time;
}

void update_delta_time(){
    calculate_delta_time(delta_time_counter);
}

void start_delta_time(){
    start_frame_timer(delta_time_counter);
}

void start_render_time(){
    start_frame_timer(frame_start_time);
}
void delay_render_time(){
    delay_for_frame(frame_start_time);
}

void start_input_time(){
    start_frame_timer(frame_start_time_input);
}
void delay_input_time(){
    delay_for_frame(frame_start_time_input);
}
