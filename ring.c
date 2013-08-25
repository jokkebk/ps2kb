#include "ring.h"

// Ring is not thread-safe, but as enqueue manipulates tail and dequeue
// head, we should be OK. Unqueue is a bit risky, though so it should be
// handled in the same side of code as dequeue.

uint8_t ringDequeue(volatile RingBuffer *ring) {
	uint8_t val = ringHead(*ring);

	if(ringEmpty(*ring))
		return 0;

	if(++ring->head >= RING_SIZE)
		ring->head = 0;

	ring->size--;

	return val;
}

uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte) {
	if(ringFull(*ring))
		return 0;

	ringTail(*ring) = byte;

	ring->size++;
	ring->tail++;

	if(ring->tail >= RING_SIZE)
		ring->tail = 0;

	return 1;
}

// Revert ringDequeue by putting back last dequeued byte to front
void ringUnqueue(volatile RingBuffer *ring) {
	if(ring->head)
		ring->head--;
	else
		ring->head = RING_SIZE - 1;

	ring->size++;
}

