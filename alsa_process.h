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

#ifndef ALSA_PROCESS_H
#define ALSA_PROCESS_H

#include <alsa/asoundlib.h>
#include "globals.h"

void setup_ALSA_mixer();
void shutdown_ALSA_mixer();
snd_mixer_elem_t *setup_ALSA_mixer_elem(char *mixer_scontrol);
void update_alsa(control_t * c);

#endif
