#include "alsa_process.h"
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include "globals.h"

snd_mixer_t *handle = NULL;
const char *card = "default";

void setup_ALSA() {
        int err;
        err = snd_mixer_open(&handle, 0);
        if (err) ERR("Error opening mixer: %s.", err);
        snd_mixer_attach(handle, card);
        snd_mixer_selem_register(handle, NULL, NULL);
        snd_mixer_load(handle);

}

void shutdown_ALSA() {
    snd_mixer_close(handle);
}

int alsa_set_mixer(char *mixer_scontrol, int step, int val) {
        long setting, min, max;
        char name[64] = {0};
        snd_mixer_selem_id_t *sid;

        strncpy(name, mixer_scontrol, 64);

        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, name);
        snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
        
        snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
        snd_mixer_selem_get_playback_dB(elem, 0xffff, &setting);         
        int err = snd_mixer_selem_set_playback_dB_all(elem, setting + (step * val), val);
        ERR("alsa error: %s for setting %d", snd_strerror(err), setting + (step * val));
}

int alsa_toggle_mute(char * mixer_scontrol){
}
