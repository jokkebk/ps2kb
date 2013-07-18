#ifndef __RING_H
#define __RING_H

#include <avr/io.h>

#define RING_SIZE 6

typedef struct {
	uint8_t buffer[RING_SIZE];
	uint8_t head, tail;
} RingBuffer;

#define ringEmpty(r) ((r).head == (r).tail)
#define ringHead(r) ((r).buffer[(r).head])
#define ringTail(r) ((r).buffer[(r).tail])

static inline void ringClear(volatile RingBuffer *ring) {
	ring->head = ring->tail = 0;
}

uint8_t ringDequeue(volatile RingBuffer *ring);
uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte);

#endif
