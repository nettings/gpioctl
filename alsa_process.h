#ifndef ALSA_PROCESS_H
#define ALSA_PROCESS_H
#include <alsa/asoundlib.h>

void setup_ALSA();
void shutdown_ALSA();
snd_mixer_elem_t* setup_ALSA_mixer_handle(char *mixer_scontrol);
int set_ALSA_volume(snd_mixer_elem_t* elem, int step, int val);
int set_ALSA_mute(snd_mixer_elem_t* elem, int val);

#endif
