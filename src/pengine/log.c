#include "log.h"

#include <stdio.h>

void pe_log(const char* text){

	const char* path = "/storage/emulated/0/Download/test.txt";
	const char* path2 = "/sdcard/Download/cube.gltf";

	FILE* new_file = fopen(path,"a");
	fputs(text,new_file);
	fclose(new_file);

    FILE* file = fopen(path2,"r");
    if(file == NULL){
        LOG("error to load: %s\n", path2);
     //   return -1;
    }
	
}
