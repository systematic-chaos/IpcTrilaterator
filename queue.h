/* Header file for queue implementation */

#ifndef QUEUE_H
#define QUEUE_H

/* Simple boolean datatype */
#define TRUE	1
#define FALSE	0
typedef int bool;

#define QUEUE_SIZE 10

typedef struct {
	double q[QUEUE_SIZE+1];	/* body of queue */
	int first;				/* position of first element */
	int last;				/* position of last element */
	unsigned int count;		/* number of queue elements */
} queue;

void initQueue(queue *q);
void enqueue(queue *q, double x);
double dequeue(queue *q);
bool empty(queue *q);
void printQueue(queue *q);

#endif

