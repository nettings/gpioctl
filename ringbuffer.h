#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stddef.h>
void setup_ringbuffer();
void shutdown_ringbuffer();
int ringbuffer_write(unsigned char msg[], size_t size);
int ringbuffer_read(unsigned char msg[], size_t size);

#endif
