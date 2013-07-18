#include "ring.h"

uint8_t ringDequeue(volatile RingBuffer *ring) {
	uint8_t val = ringHead(*ring);

	if(++ring->head >= RING_SIZE)
		ring->head = 0;

	return val;
}

uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte) {
	uint8_t index = ring->tail + 1;

	if(index >= RING_SIZE)
		index = 0;

	if(index == ring->head)
		return 0; // ring buffer full

	ring->buffer[ring->tail] = byte;
	ring->tail = index;

	return 1;
}

