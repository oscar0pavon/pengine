#ifndef EDITOR_BLENDER_H
#define EDITOR_BLENDER_H

char editing_blender_file_path[30];
bool is_editing_blender_file;
bool is_blender_file_saved;
bool blender_file_checked;

#include <engine/window_manager.h>
#include "../../engine/log.h"

static void blender_loop_checker(){
    if( window_editor_main->focus){
       

        if(is_editing_blender_file){
            if(!blender_file_checked){
            int result = system("blender -b ../assets/test_blender.blend --python-exit-code 2 --disable-abort-handler -P ../scripts/Blender/check_is_saved.py");
            if(result != 0){
                LOG("Edited \n");
            }
            if(is_blender_file_saved){
                LOG("reimport mesh blender file\n");
            }
            blender_file_checked = true;
            }
            
        }
    }else{
        blender_file_checked = false;
       
    }
}

#endif // !EDITOR_BLENDER_H
