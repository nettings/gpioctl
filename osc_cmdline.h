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

#ifndef OSC_CMDLINE_H
#define OSC_CMDLINE_H

#include "globals.h"

void help_rotary_OSC();
int parse_cmdline_rotary_OSC(control_t* c, char *config[]);
void help_switch_OSC();
int parse_cmdline_switch_OSC(control_t* c, char *config[]);

#endif
