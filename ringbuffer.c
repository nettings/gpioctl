#include "ringbuffer.h"
#include <jack/ringbuffer.h>
#include <pthread.h>
#include <stdio.h>

// event ringbuffer and lock
#define BUF_SIZE 128

static jack_ringbuffer_t *buf;
static pthread_mutex_t buflock = PTHREAD_MUTEX_INITIALIZER;

void setup_ringbuffer()
{
	buf = jack_ringbuffer_create(BUF_SIZE);
	jack_ringbuffer_mlock(buf);
}

void shutdown_ringbuffer()
{
	jack_ringbuffer_free(buf);
}

int ringbuffer_write(unsigned char *msg, size_t size)
{
	int nbytes;
	pthread_mutex_lock(&buflock);
	nbytes = jack_ringbuffer_write(buf, msg, size);
	pthread_mutex_unlock(&buflock);
	return nbytes;
}

int ringbuffer_read(unsigned char *msg, size_t size)
{
	return jack_ringbuffer_read(buf, msg, size);
}
