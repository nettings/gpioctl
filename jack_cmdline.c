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

#include "jack_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"

void help_rotary_JACK()
{
	printf("      ...,jack,cc,[ch[,min[,max[,step[,default]]]]]\n");
	printf
	    ("               cc:      MIDI continous controller number (0-%d)\n",
	     MAXCC);
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf
	    ("               min:     minimum controller value (0-%d), default 0\n",
	     MAXCCVAL);
	printf
	    ("               max:     maximum controller value (0-%d), default %d\n",
	     MAXCCVAL, MAXCCVAL);
	printf
	    ("               step:    the step size per 'click'(1-%d), default 1\n",
	     MAXCCVAL);
	printf
	    ("               default: the initial value, default is 'min'\n\n");
}

int parse_cmdline_rotary_JACK(control_t * c, char *config[])
{
	c->target = JACK;
	c->midi_cc = atoi(config[3]);
	if (c->midi_cc < 0 || c->midi_cc > MAXCC) {
		ERR("MIDI CC value out of range.");
		return -1;
	}
	if (config[4] == NULL) {
		c->midi_ch = 0;
	} else {
		c->midi_ch = atoi(config[4]) - 1;
		if (c->midi_ch < 0 || c->midi_ch > MAXMIDICH) {
			ERR("MIDI channel value out of range.");
			return -1;
		}
	}
	if (config[5] == NULL) {
		c->min = 0;
	} else {
		c->min = atoi(config[5]);
		if (c->min < 0 || c->min > MAXCCVAL) {
			ERR("min value out of range.");
			return -1;
		}
	}
	if (config[6] == NULL) {
		c->max = MAXCCVAL;
	} else {
		c->max = atoi(config[6]);
		if (c->max < 0 || c->min > MAXCCVAL) {
			ERR("max value out of range.");
			return -1;
		}
	}
	if (config[7] == NULL) {
		c->step = 1;
	} else {
		c->step = atoi(config[7]);
		if (c->step < 1 || c->step > MAXCCVAL) {
			ERR("step value out of range.");
			return -1;
		}
	}
	if (config[8] == NULL) {
		c->value = c->min;
	} else {
		c->value = atoi(config[8]);
		if (c->value < c->min || c->value > c->max) {
			ERR("default value out of range.");
			return -1;
		}
	}
	if (config[9] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	return 0;
}

void help_switch_JACK()
{
	printf("      ...,jack,cc,[ch[,toggle[,min[,max[,default]]]]]\n");
	printf
	    ("               cc:      MIDI continous controller number (0-120)\n");
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf
	    ("               toggle:  can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf
	    ("               min:     controller value when open (0-%d), default 0\n",
	     MAXCCVAL);
	printf
	    ("               max:     controller value when closed (0-%d), default %d\n",
	     MAXCCVAL, MAXCCVAL);
	printf
	    ("               default: the initial value, default is 'min'\n\n");
}

int parse_cmdline_switch_JACK(control_t * c, char *config[])
{
	c->target = JACK;
	c->midi_cc = atoi(config[2]);
	if (c->midi_cc < 0 || c->midi_cc > MAXCC) {
		ERR("MIDI CC value out of range.");
		return -1;
	}
	if (config[3] == NULL) {
		c->midi_ch = 0;
	} else {
		c->midi_ch = atoi(config[3]) - 1;
		if (c->midi_ch < 0 || c->midi_ch > MAXMIDICH) {
			ERR("MIDI channel value out of range.");
			return -1;
		}
	}
	if (config[4] == NULL) {
		c->toggle = 0;
	} else {
		c->toggle = atoi(config[4]);
		if (c->toggle != 0 && c->toggle != 1) {
			ERR("toggle must be 0 or 1.");
			return -1;
		}
	}
	if (config[5] == NULL) {
		c->min = 0;
	} else {
		c->min = atoi(config[5]);
		if (c->min < 0 || c->min > MAXCCVAL) {
			ERR("min value out of range.");
			return -1;
		}
	}
	if (config[6] == NULL) {
		c->max = MAXCCVAL;
	} else {
		c->max = atoi(config[6]);
		if (c->max < 0 || c->max > MAXCCVAL) {
			ERR("max value out of range.");
		}
	}
	if (config[7] == NULL) {
		c->value = c->min;
	} else {
		c->value = atoi(config[7]);
		if (c->value < c->min || c->value > c->max) {
			ERR("default value out of range.");
			return -1;
		}
	}
	if (config[8] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	return 0;
}
