/* Implementation of a FIFO queue abstract data type */

#include <stdio.h>

#include "queue.h"

void initQueue(queue *q) {
	q->first = 0;
	q->last = QUEUE_SIZE - 1;
	q->count = 0;
}

void enqueue(queue *q, double x) {
	if (q->count >= QUEUE_SIZE)
		printf("Warning: queue overflow enqueue x=%f\n", x);
	else {
		q->last = (q->last + 1) % QUEUE_SIZE;
		q->q[q->last] = x;
		q->count = q->count + 1;
	}
}

double dequeue(queue *q) {
	double x;
	
	if (q->count <= 0)
		printf("Warning: empty queue dequeue.\n");
	else {
		x = q->q[q->first];
		q->first = (q->first + 1) % QUEUE_SIZE;
		q->count = q->count - 1;
	}
	
	return x;
}

bool empty(queue *q) {
	return q->count <= 0;
}

void printQueue(queue *q) {
	int i;
	
	i = q->first;
	
	while (i != q->last) {
		printf("%f.9 ", q->q[i]);
		i = (i + 1) % QUEUE_SIZE;
	}
	
	printf("%f.9\n", q->q[i]);
}

