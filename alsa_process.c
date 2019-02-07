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

snd_mixer_elem_t* setup_ALSA_mixer_handle(char *mixer_scontrol) {
        char name[64] = {0};
        snd_mixer_selem_id_t *sid;
        snd_mixer_elem_t* elem;
        strncpy(name, mixer_scontrol, 64);
        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, name);
        elem = snd_mixer_find_selem(handle, sid);
        if (elem == NULL) {
                ERR("ALSA error: could not find mixer simple element %s.", mixer_scontrol);
        }
        return elem;
}

int set_ALSA_mixer_elem(snd_mixer_elem_t* elem, int step, int val) {
        int err;
        const int nch = 2;
        const int chlist[2] = { SND_MIXER_SCHN_FRONT_LEFT, SND_MIXER_SCHN_FRONT_RIGHT };
        long chval[2] = {42};
        long min = 0;
        long max = 0;
        
        snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
        for (int i=0; i<nch; i++) {
                err = snd_mixer_selem_get_playback_dB(elem, chlist[i], &chval[i]);         
                if (err) {
                        ERR("ALSA error getting value for %s ch %d: %s.", snd_mixer_selem_get_name(elem), i, snd_strerror(err));
                } else {
                        DBG("ALSA reports current setting for %s ch %d as %d milliBel [%d..%d]", snd_mixer_selem_get_name(elem), i, chval[i], min, max);
                }
                chval[i] += (step * val * 100);
                err = snd_mixer_selem_set_playback_dB(elem, chlist[i], chval[i], val);
                if (err) {
                        ERR("ALSA error: %s while setting %s to %d milliBel", snd_strerror(err), snd_mixer_selem_get_name(elem), chval[i]);
                } else {
                        DBG("ALSA reports %s while setting %s to %d milliBel", snd_strerror(err), snd_mixer_selem_get_name(elem), chval[i]);
                }
        }
}

int toggle_ALSA_mute(snd_mixer_elem_t* elem){
}

