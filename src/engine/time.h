#ifndef TIME_H
#define TIME_H


typedef struct timespec PTime;

void start_render_time();
void delay_render_time();

void start_input_time();
void delay_input_time();

void start_delta_time();
void update_delta_time();

void start_frame_timer(PTime time);

extern double delta_time;
#endif
