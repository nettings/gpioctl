/*
  gpioctl

  Copyright (C) 2019 Jörn Nettingsmeier

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "alsa_process.h"
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#include "globals.h"

static snd_mixer_t *mixer_handle = NULL;
char alsa_card[MAXNAME] = ALSA_CARD;

void setup_ALSA_mixer()
{
	char name[64] = { 0 };

	int err;
	err = snd_mixer_open(&mixer_handle, 0);
	if (err)
		ERR("Error opening mixer: %s.", snd_strerror(err));
	snd_mixer_attach(mixer_handle, alsa_card);
	snd_mixer_selem_register(mixer_handle, NULL, NULL);
	snd_mixer_load(mixer_handle);
}

void shutdown_ALSA_mixer()
{
	snd_mixer_close(mixer_handle);
}

snd_mixer_elem_t *setup_ALSA_mixer_elem(char *mixer_scontrol)
{
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, mixer_scontrol);
	elem = snd_mixer_find_selem(mixer_handle, sid);
	if (elem == NULL) {
		ERR("ALSA error: could not find mixer simple element %s.",
		    mixer_scontrol);
	}
	return elem;
}

int set_ALSA_volume(snd_mixer_elem_t * elem, int val)
{
	int err;
	long cval;

	snd_mixer_handle_events(mixer_handle);	// make sure we're aware of mixer changes from elsewhere (https://www.raspberrypi.org/forums/viewtopic.php?p=1165130)
	err =
	    snd_mixer_selem_get_playback_dB(elem, 0,
					    &cval);
	if (err) {
		ERR("ALSA error getting value for %s: %s.",
		    snd_mixer_selem_get_name(elem), snd_strerror(err));
	} else {
		DBG("ALSA reports current setting for %s as %d milliBel.",
		    snd_mixer_selem_get_name(elem), cval);
	}
	val += cval;
	err = snd_mixer_selem_set_playback_dB_all(elem, val, 1);
	if (err) {
		ERR("ALSA error: %s while setting %s to %d milliBel.",
		    snd_strerror(err), snd_mixer_selem_get_name(elem), val);
	} else {
		DBG("ALSA reports %s while setting %s to %d milliBel.",
		    snd_strerror(err), snd_mixer_selem_get_name(elem), val);
	}
	return 0;
}

int set_ALSA_mute(snd_mixer_elem_t * elem, int val)
{
	int err;
	int cval;

	snd_mixer_handle_events(mixer_handle);	// make sure we're aware of mixer changes from elsewhere (https://www.raspberrypi.org/forums/viewtopic.php?p=1165130)
	err =
	    snd_mixer_selem_get_playback_switch(elem, 0,
						&cval);
	if (err) {
		ERR("ALSA error getting value for %s: %s.",
		    snd_mixer_selem_get_name(elem), snd_strerror(err));
	} else {
		DBG("ALSA reports current setting for %s as %d.",
		    snd_mixer_selem_get_name(elem), cval);
	}
	val = 1 - cval;
	err = snd_mixer_selem_set_playback_switch_all(elem, val);
	if (err) {
		ERR("ALSA error: %s while setting %s to %d.", snd_strerror(err),
		    snd_mixer_selem_get_name(elem), val);
	} else {
		DBG("ALSA reports %s while setting %s to %d.",
		    snd_strerror(err), snd_mixer_selem_get_name(elem), val);
	}
}
