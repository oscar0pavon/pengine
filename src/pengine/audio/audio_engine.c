#include "audio_engine.h"
#include <pengine/engine.h>

#include "../audio.h"


#include <pulse/simple.h>
#include <pulse/error.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


/*Add audio type to audio thread sounds queue*/
void pe_audio_play(PAudio* audio){


}
void pe_audio_pulse(){

    /* The Sample format to use */
    static const pa_sample_spec sample_specs = {
        .format = PA_SAMPLE_S16LE,
        .rate = 44100,
        .channels = 2
    };
    
    pa_simple *stream = NULL;

    int error;
    stream = pa_simple_new(NULL,"test audio",PA_STREAM_PLAYBACK,NULL,"playback",&sample_specs,NULL,NULL,&error);
    if(stream == NULL) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    }

    File effect;
	load_file("/home/pavon/cat.wav", &effect);

    File music;
	load_file("/home/pavon/test.wav", &music);

    for (;;) {
        uint8_t buffer[1024];
        ssize_t bytes_readed;

		if ((bytes_readed = file_read(&effect, buffer, sizeof(buffer))) == 0) {

            if (bytes_readed == 0) /* EOF */
                break;
        }
        if (pa_simple_write(stream, buffer, (size_t) bytes_readed, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
        }
        //LOG("playing\n");
    }
}

char pe_byte_add(char byte, char byte2){

            char byte_result;
            ZERO(byte_result);
            char high_byte;
            char low_byte;
            ZERO(low_byte);
            ZERO(high_byte);

            int result = memcmp(&byte,&byte2,1);
            if(result > 0) {//byte is greater
                high_byte = byte;
                low_byte = byte2;
            }else{  
                high_byte = byte2;
                low_byte = byte;
            }
            
            byte_result = (high_byte << 8) | low_byte;

            return byte_result;
}

char pe_audio_byte_mix(char byte , char byte2){

            char added_byte = pe_byte_add(byte,byte2);

            int mult = (int) byte * (int) byte2;
            int div = mult / 256;
            int mix = (int)added_byte - div;
            char mix_c = (char) mix;
}
void pe_audio_mix_two_file(char* out,char* in){
        char tmp[buff_size];
        ZERO(tmp);
        memcpy(&tmp,out,buff_size);
        for(int i = 0; i < buff_size; i++){
            char byte = in[i];
            char byte2 = tmp[i];
            
            out[i] = pe_audio_byte_mix(byte,byte2);

        }
}
void pe_audio_mix(){
    File audio1;
    File audio2;
    
	load_file( "/home/pavon/test.wav", &audio1);
	load_file( "/home/pavon/cat.wav", &audio2);


	buff_size = frames * channels * 2 /* 2 -> sample size */;
	buff = (char *) malloc(buff_size);
    
    char * buffer1 = (char*) malloc(buff_size);

    char * buffer2 = (char*) malloc(buff_size);

    int bytes_reader1 = 0;
    int bytes_readed2 = 0;

	for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {


        bytes_reader1 = file_read(&audio1,buffer1,buff_size);
        if(bytes_reader1 == 0){
            LOG("end play\n");
            break;
        }

        bytes_readed2 = file_read(&audio2,buffer2,buff_size);
        
        char mix_buffer[buff_size];
        ZERO(mix_buffer);

        for(int i = 0; i < buff_size; i++){
            char byte = buffer1[i];
            char byte2 = buffer2[i];
            
            mix_buffer[i] = pe_audio_byte_mix(byte,byte2);

        }

        if (pcm = snd_pcm_writei(pcm_handle, mix_buffer, frames) == -EPIPE) {
			printf("XRUN.\n");
			snd_pcm_prepare(pcm_handle);
		} else if (pcm < 0) {
			printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
		}

	}
	
    close_file(&new_audio_file);
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buff);

}
void pe_audio_play_master(){
	
    buff_size = frames * channels * 2 /* 2 -> sample size */;
    int count_play = 0;
            if(pe_audio_array_queue.count < 1){
                return;
            }
            count_play = 0;

            for(int i = 0; i < pe_audio_array_queue.count; i++){
        
                PAudio* audio = array_get_pointer(&pe_audio_array_queue,i);

                if(audio->finish == true) continue;
               
                count_play++;
                
                    if(count_play == 1 ){
                        file_read(&audio->file,play_buffer,buff_size);
                        continue;
                    } 
                    
                    char buffer1[buff_size];

                    int readed = file_read(&audio->file,buffer1,buff_size);
                    if(readed == 0){
                        audio->playing = false;
                        audio->finish = true;
                        LOG("end cat\n");
                    }

                    pe_audio_mix_two_file(play_buffer,buffer1);
                
            }

            int readed = snd_pcm_writei(pcm_handle, play_buffer, frames);
            if (readed == -EPIPE) {
		    	printf("XRUN.\n");
		    	snd_pcm_prepare(pcm_handle);
	    	} else if (pcm < 0) {
			    printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
	    	}

	    

    
}


void pe_audio_load(PAudio* audio,char* path){
    ZERO(*(audio));
	load_file( path, &audio->file);
    char buff[sizeof(struct PWaveHeader)];
    file_read(&audio->file,buff,sizeof(struct PWaveHeader));
    memcpy(&audio->header,buff,sizeof(struct PWaveHeader));
    LOG("Frame ratei %u\n",audio->header.sample_rate);
    double duration = audio->header.overall_size / audio->header.sample_rate;

    LOG("Durarion %f\n",duration);


}

void audio_engine_main_thread(void* argument){

    LOG("Audio Engine [OK]\n");


    pe_audio_alsa_init();
    array_init(&pe_audio_array_queue,sizeof(void*),100);

	pe_audio_load(&audio1, "/home/pavon/test.wav");
	pe_audio_load(&audio2, "/home/pavon/cat.wav");

     play_buffer = (char*) malloc(buff_size);
    array_add_pointer(&pe_audio_array_queue,&audio1);


    //pe_audio_mix();

	while(1){
		pe_thread_control(&thread_audio_commads);
        pe_audio_play_master();
        
	}	
}

int pe_audio_init(){
	thread_new_detached(&audio_engine_main_thread,NULL,"Audio",&pe_th_audio_id);
	
}

