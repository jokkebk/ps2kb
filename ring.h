#ifndef __RING_H
#define __RING_H

#include <avr/io.h>

#define RING_SIZE 16

typedef struct {
	uint8_t buffer[RING_SIZE];
	uint8_t head, tail;
	uint8_t size;
} RingBuffer;

#define ringEmpty(r) ((r).size == 0)
#define ringFull(r) ((r).size == RING_SIZE)

#define ringHead(r) ((r).buffer[(r).head])
#define ringTail(r) ((r).buffer[(r).tail])

static inline void ringClear(volatile RingBuffer *ring) {
	ring->head = ring->tail = ring->size = 0;
}

// Dequeue item. Call only if ring is not empty!
uint8_t ringDequeue(volatile RingBuffer *ring);

// Put last dequed item back, if possible
void ringUnqueue(volatile RingBuffer *ring);

// Add item to end of queue, if possible
uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte);

#endif
