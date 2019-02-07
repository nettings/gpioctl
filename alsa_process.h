#ifndef ALSA_PROCESS_H
#define ALSA_PROCESS_H

int alsa_set_mixer(char * mixer_scontrol, int step, int val);
int alsa_toggle_mute(char * mixer_scontrol);
void setup_ALSA();
void shutdown_ALSA();

#endif

