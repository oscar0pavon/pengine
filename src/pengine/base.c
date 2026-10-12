#include "engine.h"

//INFO called once, after pengine_run()'s main loop has already exited -
//pe_terminate() is what breaks that loop by clearing pe_running. tearing
//vulkan down needs the device idle first: the loop's last frame can still be
//in flight on the GPU when pe_running goes false
void pe_end(){
    vkDeviceWaitIdle(vk_device);
    pe_vk_end();
    clear_engine_memory();
}

void pe_init() {
  LOG("Initializing pengine\n");
  pe_init_memory(INIT_MEMORY); //VERY IMPORTANT

  pe_init_arrays();

  pe_init_global_variables();

  pe_change_background_color(1, 0, 0, 1);

  pe_th_main_id = pthread_self();

  pe_vk_initialized = false;
  // pe_audio_init();
  // pe_phy_init();
  pe_input_init();

  LOG("pengine initialized\n");

 

}
