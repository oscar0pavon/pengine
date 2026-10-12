//
// Created by pavon on 6/23/19.
//
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "audio.h"

#include "../../Engine/file_loader.h"
#include "../../Engine/utils.h"
#include "../../Engine/log.h"

SLObjectItf engine_object;
SLEngineItf audio_engine;
SLObjectItf output_mix_object;

SLObjectItf player_object_interface;
SLPlayItf player_interface;
SLSeekItf player_seek_interface;


void load_audio(const char* path){
    File audio_file;
    load_file(path, &audio_file);



}
void player_background_music() {
    File audio_file;
    load_file("music.wav", &audio_file);

    SLDataLocator_AndroidFD data_locator_in;
    data_locator_in.locatorType = SL_DATALOCATOR_ANDROIDFD;
    data_locator_in.fd = audio_file.resource_descriptor.descriptor;
    data_locator_in.offset = audio_file.resource_descriptor.start;
    data_locator_in.length = audio_file.resource_descriptor.length;

    SLDataFormat_MIME data_format;
    data_format.formatType = SL_DATAFORMAT_MIME;
    data_format.mimeType = NULL;
    data_format.containerType = SL_CONTAINERTYPE_UNSPECIFIED;

    SLDataSource data_source;
    data_source.pLocator = &data_locator_in;
    data_source.pFormat = &data_format;

    SLDataLocator_OutputMix data_locator_output;
    data_locator_output.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    data_locator_output.outputMix = output_mix_object;

    SLDataSink data_sink;
    data_sink.pLocator = &data_locator_output;
    data_sink.pFormat = NULL;

    const SLuint32  player_count = 2;
    const SLInterfaceID player_interface_id[] = {SL_IID_PLAY, SL_IID_SEEK};
    const SLboolean  player_reqs[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    (*audio_engine)->CreateAudioPlayer(audio_engine,&player_object_interface,&data_source, &data_sink,player_count,player_interface_id,player_reqs);

    (*player_object_interface)->Realize(player_object_interface,SL_BOOLEAN_FALSE);

    (*player_object_interface)->GetInterface(player_object_interface,SL_IID_PLAY,&player_interface);

    (*player_object_interface)->GetInterface(player_object_interface, SL_IID_SEEK, &player_seek_interface);

    //play

    (*player_seek_interface)->SetLoop(player_seek_interface,SL_BOOLEAN_TRUE, 0 , SL_TIME_UNKNOWN);
    (*player_interface)->SetPlayState(player_interface,SL_PLAYSTATE_PLAYING);


}

void init_audio_manager(){


    SLresult result;

    const SLuint32 engine_mix_interface_id_count = 1;
    const SLInterfaceID engine_mix_interface_ids[] = {SL_IID_ENGINE};
    const SLboolean  engine_mix_reqs[] = {SL_BOOLEAN_TRUE};
    const SLuint32  output_mix_interface_id_count = 0;
    const SLInterfaceID output_mix_interfaces_ids[] = {};
    const SLboolean output_mix_reqs[] = {};

    result = slCreateEngine(&engine_object, 0 , NULL , engine_mix_interface_id_count, engine_mix_interface_ids, engine_mix_reqs);
    if(result != SL_RESULT_SUCCESS){
        LOG("Audio engine initialize failed");
    }

    (*engine_object)->Realize(engine_object,SL_BOOLEAN_FALSE);
    (*engine_object)->GetInterface(engine_object, SL_IID_ENGINE, &audio_engine);

    (*audio_engine)->CreateOutputMix(audio_engine,&output_mix_object, output_mix_interface_id_count, output_mix_interfaces_ids, output_mix_reqs);
    (*output_mix_object)->Realize(output_mix_object, SL_BOOLEAN_FALSE);


    //player_background_music();

};



void stop_backgrount_music(){

}