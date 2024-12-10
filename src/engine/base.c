#include "engine.h"

void pe_end(){
    engine_running = false;   
    clear_engine_memory();
}

void pe_init() {
    LOG("Initializing pengine\n");
    pe_init_memory(); //VERY IMPORTANT

    pe_init_arrays();

    pe_init_global_variables();

    pe_change_background_color(1, 0, 0, 1);

    pe_th_main_id = pthread_self();

    pe_vk_initialized = false;
    // pe_audio_init();
    // pe_phy_init();
    ZERO(input);

    LOG("pengine initialized\n");
}
