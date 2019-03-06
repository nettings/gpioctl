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

#include "ringbuffer.h"
#include <jack/ringbuffer.h>
#include <pthread.h>
#include <errno.h>
#include "globals.h"

// event ringbuffer and lock
static jack_ringbuffer_t *buf;
static pthread_mutex_t buflock = PTHREAD_MUTEX_INITIALIZER;

int setup_ringbuffer(int nbytes)
{
	DBG("Setting up ringbuffer of %d bytes.", nbytes);
	buf = jack_ringbuffer_create(nbytes);
	if (buf == NULL) {
	        ERR("Could not create JACK ringbuffer.");
	        return -ENOMEM;
        }
	jack_ringbuffer_mlock(buf);
	return 0;
}

int shutdown_ringbuffer()
{
	DBG("Shutting down ringbuffer");
	jack_ringbuffer_free(buf);
	return 0;
}

int ringbuffer_write(unsigned char *msg, size_t size)
{
	int nbytes;
	// there can be multiple writer threads, ensure only
	// one can write at the same time:
	pthread_mutex_lock(&buflock);
	nbytes = jack_ringbuffer_write(buf, (char *)msg, size);
	pthread_mutex_unlock(&buflock);
	return nbytes;
}

int ringbuffer_read(unsigned char *msg, size_t size)
{
	return jack_ringbuffer_read(buf, (char *)msg, size);
}
