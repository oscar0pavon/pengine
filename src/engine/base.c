#include "engine.h"
#include <engine/windows_manager.h>

void pe_end(){
    engine_running = false;   
    clear_engine_memory();
}

void pe_init() {
    LOG("Initializing Pavon Engine\n");
    init_engine_memory(); // VERY IMPORTANT

    pe_init_arrays();

    pe_init_global_variables();

    pe_change_background_color(1, 0, 0, 1);

    pe_th_main_id = pthread_self();
#ifdef LINUX
    pe_vk_initialized = false;
    // pe_audio_init();
    // pe_phy_init();
    pe_input_init();

#endif
    pe_wm_init();
    LOG("Pavon Engine initialized");
}
