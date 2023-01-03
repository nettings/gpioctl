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

#include "alsa_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

void help_rotary_ALSA()
{
	printf("      ...,alsa,device,control[,step]\n");
	printf("               device: an ALSA hardware device such as hw:1\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("               step: the step size in dB per click, default 3\n");
}

int parse_cmdline_rotary_ALSA(control_t * c, char *config[])
{
	c->target = ALSA;
	if (config[3] == NULL) {
		ERR("device cannot be empty.");
		return -1;
	}
	c->param2 = strncpy(c->param2, config[3], MAXNAME);
	if (config[4] == NULL) {
		ERR("control cannot be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[4], MAXNAME);
	if (config[5] == NULL) {
		c->step = 3;
	} else {
		c->step = atoi(config[5]);
	}
	if (config[6] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	c->min = -100;
	c->max = 0;
	c->value = c->min;

	return 0;
}

void help_switch_ALSA()
{
	printf("      ...,alsa,device,control\n");
	printf("               device:  the name of an ALSA hardware device, such as hw:1\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("                        (switch will operate the MUTE function)\n");
}

int parse_cmdline_switch_ALSA(control_t * c, char *config[])
{
	c->target = ALSA;
	if (config[2] == NULL) {
		ERR("device cannot be empty.");
		return -1;
	}
	c->param2 = strncpy(c->param2, config[2], MAXNAME);
	if (config[3] == NULL) {
		ERR("control cannot be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[3], MAXNAME);
	if (config[4] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	c->min = 0;
	c->max = 1;
	c->value = 0;
	c->toggle = 1;
	return 0;
}
