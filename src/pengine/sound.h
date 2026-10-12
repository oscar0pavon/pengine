#ifndef PE_SOUND_H
#define PE_SOUND_H

#include <stdbool.h>

//a sound on the default output device, played by miniaudio. wav, mp3, flac and
//ogg are read, and every sound that is playing is mixed into the output
typedef struct PSound {
  //a ma_sound, which is kept out of this header so that what includes it does
  //not parse miniaudio
  void *handle;
} PSound;

//opens the output device. false when there is none, and then every call below
//does nothing, so a machine without audio runs the game silent
bool pe_sound_init(void);
void pe_sound_end(void);

//a short sound is decoded whole now, a long one (music) is streamed from the
//file as it plays
bool pe_sound_load(PSound *sound, const char *path, bool stream);
void pe_sound_free(PSound *sound);

//play starts from the beginning, also when it is playing already
void pe_sound_play(PSound *sound);
void pe_sound_stop(PSound *sound);
bool pe_sound_is_playing(const PSound *sound);

void pe_sound_set_looping(PSound *sound, bool looping);
//1 is the level of the file
void pe_sound_set_volume(PSound *sound, float volume);
//1 is the pitch of the file; 2 is an octave up, and plays twice as fast
void pe_sound_set_pitch(PSound *sound, float pitch);

#endif // !PE_SOUND_H
