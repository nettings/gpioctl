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

#include "osc_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "globals.h"

void help_rotary_OSC()
{
	printf("       ...,osc,url,path,min,max,step,default\n");
	printf("               url:     The OSC url of the receiver(s), such as\n");
	printf("                        osc.udp://239.0.2.149:7000\n");
	printf("               min:     minimum value (%d - %d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d - %d), default 100\n", INT_MIN, INT_MAX);
	printf("               step:    the step size per click, default 1\n");
	printf("               default: the initial value, default is 'min'\n\n");
}

int parse_cmdline_rotary_OSC(control_t * c, char *config[])
{
	c->target = OSC;
	c->param1 = strncpy(c->param1, config[3], MAXNAME);
	if (strlen(c->param1) < 1) {
		ERR("url cannot be empty");
		return -1;
	}
	if (config[4] == NULL) {
		ERR("path cannot be empty");
		return -1;
	} else {
		c->param2 = strncpy(c->param2, config[4], MAXNAME);
	}
	if (config[5] == NULL) {
		c->min = 0;
	} else {
		c->min = atoi(config[5]);
	}
	if (config[6] == NULL) {
		c->max = 100;
	} else {
		c->max = atoi(config[6]);
	}
	if (config[7] == NULL) {
		c->step = 1;
	} else {
		c->step = atoi(config[7]);
	}
	if (config[8] == NULL) {
		c->value = c->min;
	} else {
		c->value = atoi(config[8]);
	}
	if (config[9] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	return 0;
}

void help_switch_OSC()
{
	printf("       ...,osc,url,path,toggle,min,max,default\n");
	printf
	    ("               url:     An OSC url, such as osc.udp://239.0.2.149/gpioctl/level\n");
	printf("               path:    An OSC path, such as /mixer/level\n");
	printf
	    ("               toggle:  can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     value when open (%d - %d), default 0\n",
	       INT_MIN, INT_MAX);
	printf
	    ("               max:     value when closed (%d - %d), default 100\n",
	     INT_MIN, INT_MAX);
	printf
	    ("               default: the initial value, default is 'min'\n\n");
}

int parse_cmdline_switch_OSC(control_t * c, char *config[])
{
	c->target = OSC;
	c->param1 = strncpy(c->param1, config[2], MAXNAME);
	if (strlen(c->param1) < 1) {
		ERR("url cannot be empty");
		return -1;
	}
	if (config[3] == NULL) {
		ERR("path cannot be empty");
		return -1;
	} else {
		c->param2 = strncpy(c->param2, config[3], MAXNAME);
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
	}
	if (config[6] == NULL) {
		c->max = 100;
	} else {
		c->max = atoi(config[6]);
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
