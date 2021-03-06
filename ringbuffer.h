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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stddef.h>

int setup_ringbuffer(int nbytes);
int shutdown_ringbuffer();
int ringbuffer_write(unsigned char msg[], size_t size);
int ringbuffer_read(unsigned char msg[], size_t size);

#endif
