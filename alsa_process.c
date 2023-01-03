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


int setup_ALSA()
{
	//noop, now done per control
}

int shutdown_ALSA(control_t **controllers)
{
	DBG("Shutting down ALSA handles.");
	control_t *c;
	for (int i=0; i<NCONTROLLERS; i++) {
		DBG("Controller #%d:",i);
		c = controllers[i];
		if (c == NULL) continue;
		DBG("\ttarget is %s, type is %s", control_targets[c->target], control_types[c->type]);
		if (c->target == ALSA || c->target == SLAVE) {
			if (c->param2 != NULL) {
				snd_mixer_close(c->param2);
				DBG("Freed mixer handle for controller #%d.", i);
			} else {
				ERR("No mixer handle in controller #%d.", i);
			}
		}
	}
	return 0;
}

snd_mixer_elem_t *setup_ALSA_elem(control_t * c)
{
	int err;
	snd_mixer_t *mixer_handle;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;

	DBG("Setting up ALSA mixer handle for device %s.", c->param2);
	err = snd_mixer_open(&mixer_handle, 0);
	if (err) {
		ERR("Error opening mixer: %s.", snd_strerror(err));
		return NULL;
	}
	snd_mixer_attach(mixer_handle, c->param2);
	snd_mixer_selem_register(mixer_handle, NULL, NULL);
	snd_mixer_load(mixer_handle);

	DBG("Getting ALSA mixer handle for %s.", c->param1);
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, c->param1);
	elem = snd_mixer_find_selem(mixer_handle, sid);
	if (elem == NULL) {
		ERR("ALSA error: could not find mixer simple element %s.", c->param1);
		return NULL;
	}
	// is sid now expendable? or do we have to hold on to it while we use elem?
	//snd_mixer_selem_id_free(sid);
	// store mixer handle in control structure for use
	c->param2 = (snd_mixer_t *) mixer_handle;
	return elem;
}

int update_ALSA(control_t * c)
{
	int err;

	DBG("Setting mixer element %s to %d.", snd_mixer_selem_get_name(c->param1), c->value);
	switch (c->type) {
	case ROTARY:
		// ALSA handles level in milliBel!
		err = snd_mixer_selem_set_playback_dB_all(c->param1, c->value * 100, 1);
		// set_ALSA_volume(c->param1, val * c->step * 100);
		break;
	case SWITCH:
		err = snd_mixer_selem_set_playback_switch_all(c->param1, c->value);
		// set_ALSA_mute(c->param1, val);
		break;
	default:
		ERR("Unknown c->type %d. THIS SHOULD NEVER HAPPEN.", c->type);
		return -EINVAL;
		break;
	}
	if (err) {
		ERR("ALSA error: %s while setting %s to %d.", snd_strerror(err),
		    snd_mixer_selem_get_name(c->param1), c->value);
		    return err;
	}
	return 0;
}

int get_ALSA_value(control_t* c)
{
	int err, res;
	long i;

	// make sure we're aware of mixer changes from elsewhere
	// (https://www.raspberrypi.org/forums/viewtopic.php?p=1165130)
	snd_mixer_handle_events(c->param2);
        switch (c->type) {
        case ROTARY:
                // ALSA handles level in milliBel!
                err = snd_mixer_selem_get_playback_dB(c->param1, 0, &i);
                res = i / 100;
                break;
        case SWITCH:
                err = snd_mixer_selem_get_playback_switch(c->param1, 0, &res);
                // set_ALSA_mute(c->param1, val);
                break;
        default:
                ERR("Unknown c->type %d. THIS SHOULD NEVER HAPPEN.", c->type);
                return -EINVAL;
                break;
        }
        if (err) {
                ERR("ALSA error: %s while reading %s.", snd_strerror(err),
                    snd_mixer_selem_get_name(c->param1));
                return err;
        }
        return res;
}
