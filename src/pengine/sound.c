#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "log.h"
#include "sound.h"

#include <stdlib.h>

static ma_engine sound_engine;
static bool sound_engine_started;

bool pe_sound_init(void) {
  if (ma_engine_init(NULL, &sound_engine) != MA_SUCCESS) {
    LOG("pe_sound_init: no audio output device\n");
    return false;
  }
  sound_engine_started = true;
  return true;
}

void pe_sound_end(void) {
  if (!sound_engine_started)
    return;
  ma_engine_uninit(&sound_engine);
  sound_engine_started = false;
}

bool pe_sound_load(PSound *sound, const char *path, bool stream) {
  sound->handle = NULL;
  if (!sound_engine_started)
    return false;

  ma_uint32 flags = MA_SOUND_FLAG_NO_SPATIALIZATION;
  flags |= stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;

  ma_sound *handle = calloc(1, sizeof(ma_sound));
  if (ma_sound_init_from_file(&sound_engine, path, flags, NULL, NULL, handle) !=
      MA_SUCCESS) {
    LOG("pe_sound_load: can't read %s\n", path);
    free(handle);
    return false;
  }
  sound->handle = handle;
  return true;
}

void pe_sound_free(PSound *sound) {
  if (!sound->handle)
    return;
  ma_sound_uninit(sound->handle);
  free(sound->handle);
  sound->handle = NULL;
}

void pe_sound_play(PSound *sound) {
  if (!sound->handle)
    return;
  ma_sound_seek_to_pcm_frame(sound->handle, 0);
  ma_sound_start(sound->handle);
}

void pe_sound_stop(PSound *sound) {
  if (sound->handle)
    ma_sound_stop(sound->handle);
}

bool pe_sound_is_playing(const PSound *sound) {
  return sound->handle && ma_sound_is_playing(sound->handle);
}

void pe_sound_set_looping(PSound *sound, bool looping) {
  if (sound->handle)
    ma_sound_set_looping(sound->handle, looping);
}

void pe_sound_set_volume(PSound *sound, float volume) {
  if (sound->handle)
    ma_sound_set_volume(sound->handle, volume);
}

void pe_sound_set_pitch(PSound *sound, float pitch) {
  if (sound->handle)
    ma_sound_set_pitch(sound->handle, pitch);
}
