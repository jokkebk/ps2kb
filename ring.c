/**
 * PS/2 keyboard implementation and knock sensor.
 * Ring buffer implementation. This implementation is not thread-safe
 * so any program using it should take care of disabling interrupts 
 * etc. if needed. However, if main program is only using Enqueue and
 * interrupt only De/Unqueue or vice versa, it should be quite safe.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see LICENSE for details.
 *
 * See README.md or http://codeandlife/?p=1488 for details.
 */
#include "ring.h"

// Initialize / clear ring
void ringClear(volatile RingBuffer *ring) {
	ring->read = ring->write = ring->buffer;
}

// Dequeue item. Call only if ring is not empty!
uint8_t ringDequeue(volatile RingBuffer *ring) {
	uint8_t val;
	
	val = *ring->read;

	if(++ring->read == ringEnd(*ring))
		ring->read = ring->buffer; // wrap

	return val;
}

// Add item to end of queue, if possible, returns 1 on success
uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte) {
	volatile uint8_t *nextWrite = ring->write + 1;

	if(nextWrite == ringEnd(*ring))
		nextWrite = ring->buffer; // wrap

	if(nextWrite == ring->read) // unacceptable - ring full
		return 0;

	*ring->write = byte;
	ring->write = nextWrite;

	return 1;
}

// Revert ringDequeue by putting back last dequeued byte to front
void ringUnqueue(volatile RingBuffer *ring) {
	if(ring->read != ring->buffer)
		ring->read--; // still room
	else
		ring->read = ring->buffer + RING_SIZE - 1;
}

