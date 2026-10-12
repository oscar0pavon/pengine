
#include "file_loader.h"
#include "utils.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



void close_file(File* file){
#ifndef ANDROID
        //free((void*)file->path);
#endif
   if(file->opened == true) 
		free(file->data);
}


int load_file(const char* path, File* output){
#ifdef ANDROID_ASSET
    AAsset* file =  AAssetManager_open(assets_manager,path,AASSET_MODE_BUFFER);
    if(!file){
        LOG("Error in loading file: %s",path);
		return -1;
    }
    output->resource_descriptor.descriptor = AAsset_openFileDescriptor(file,&output->resource_descriptor.start,&output->resource_descriptor.length);
    size_t size = AAsset_getLength(file);

    output->data = malloc(size);

    AAsset_read(file,output->data,size);

    output->size_in_bytes = size;
    output->path = path;
    
    AAsset_close(file);
#else
    
    output->path = path;
    FILE* file = fopen(path,"r");
    if(file == NULL){
        LOG("error to load: %s\n", path);
        return -1;
    }

		output->resource_descriptor.descriptor = fileno(file);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind (file);

    void* file_buffer = malloc(file_size+1);
    memset(file_buffer,0,file_size+1);

    fread(file_buffer,1, file_size, file);
    output->data = file_buffer;
    output->size_in_bytes = file_size;



    fclose(file);
#endif

	output->opened = true;
	return 0;
}

int pe_file_openb(const char* path, File* output){
  output->path = path;
  FILE* file = fopen(path,"rb");
  if(file == NULL){
      LOG("error to load: %s\n", path);
      return -1;
  }

  output->resource_descriptor.descriptor = fileno(file);

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind (file);

  void* file_buffer = malloc(file_size);

  fread(file_buffer,1, file_size, file);
  output->data = file_buffer;
  output->size_in_bytes = file_size;


  fclose(file);

	output->opened = true;
	return 0;
}

int file_read(File *file, char *buffer, int buffer_size) {
    if(file->bytes_readed < file->size_in_bytes){
        memcpy(buffer, file->data + file->bytes_readed, buffer_size);
        file->bytes_readed += buffer_size;
	    return buffer_size;
    }else return 0;
}
