#include "time.h"
#include "engine.h"

#include <sys/time.h>

struct timeval time_measured_01, time_measured_02;

float render_frame_time = 0;
float disired_frame_time = 0.016f;

u8 frames = 0;
float frame_second = 0;

void time_update_delta(){

}

void time_start(){
    gettimeofday(&time_measured_01, NULL);
   
    render_frame_time += time_delta;
}

float time_end(){
    gettimeofday(&time_measured_02, NULL);
  

    //compute
    time_elapsed_time = (time_measured_02.tv_sec - time_measured_01.tv_sec) * 1000.0;
    time_elapsed_time += (time_measured_02.tv_usec - time_measured_01.tv_usec) / 1000.0;

    //convert to seconds
    time_delta = time_elapsed_time /1000;

    float diffirence2 = (time_measured_02.tv_sec - time_measured_01.tv_sec) * (long)1e9 + (time_measured_02.tv_usec - time_measured_01.tv_usec) ;
    
    
    //frame per second

    frame_time= diffirence2 / 1000;

    frame_second += time_elapsed_time;

    if (frame_second >= 1000) {
      FPS = frames * (1000.f / frame_second);
      frames = 0;
      frame_second = 0;
    } else {
      frames++;
    }

    return 0;
}
