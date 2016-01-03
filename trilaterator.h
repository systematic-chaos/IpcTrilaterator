#ifndef TRILATERATOR_H
#define TRILATERATOR_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define SEMKEY 6534
#define SHMKEY 6535
#define MSGKEY 6536

typedef struct ssphere {
	double r;
	double x;
	double y;
	double z;
} sphere;

/* Message struct */
typedef struct msg {
	long beaconId; /* message type */
	sphere measureInfo; /* message contents */
} message;

#endif

