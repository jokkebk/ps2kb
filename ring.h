/**
 * PS/2 keyboard implementation and knock sensor.
 * Ring buffer implementation. This implementation is not thread-safe
 * so any program using it should take care of disabling interrupts 
 * etc. if needed. However, if main program is only using Enqueue and
 * interrupt only De/Unqueue or vice versa, it should be quite safe.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see License.txt for details.
 *
 * See details about this project at http://codeandlife.com/?p=1488
 */
#ifndef __RING_H
#define __RING_H

#include <avr/io.h>

// Due to enqueue implementation, size-1 items fit into ring
#define RING_SIZE 10

typedef struct {
	volatile uint8_t buffer[RING_SIZE];
	volatile uint8_t *read, *write;
} RingBuffer;

// Macros operate on actual struct, not pointer
#define ringEmpty(r) ((r).read == (r).write)

// This makes use of the fact that struct is likely non-packed and linear
#define ringEnd(r) ((uint8_t *)(&(r).read))

// Initialize / clear ring
void ringClear(volatile RingBuffer *ring);

// Dequeue item. Call only if ring is not empty!
uint8_t ringDequeue(volatile RingBuffer *ring);

// Put last dequed item back, if possible
void ringUnqueue(volatile RingBuffer *ring);

// Add item to end of queue, if possible, returns 1 on success
uint8_t ringEnqueue(volatile RingBuffer *ring, uint8_t byte);

#endif
