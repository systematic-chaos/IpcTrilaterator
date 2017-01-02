#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "trilaterator.h"
#include "queue.h"
#include "kalman.h"

void locationCoordinator(sphere* beacons, bool eager);
void *lazyMeasure(void* beaconId);
void *eagerMeasure(void* beaconId);
void trilaterate(sphere* r1, sphere* r2, sphere* r3, sphere* result);
void quatrilaterate(sphere* r1, sphere* r2, sphere* r3, sphere* r4, sphere* result);
void quitSignalHandler(int signum);

int shmId, semId, msgQueueId;
pthread_t pIds[4];
int nBeacons;

int main(int argc, char* argv[]) {
	if (argc != 3) {
		printf("trilaterator <-e|-l> <locations file>\n");
		return 1;
	}
	
	char option;
	if (strlen(argv[1]) != 2 || sscanf(argv[1], "-%c", &option) <= 0) {
		printf("trilaterator <-e|-l> <locations file>\n");
		return 1;
	}
	if (option != 'e' && option != 'l') {
		printf("trilaterator <-e|-l> <locations file>\n");
		return 1;
	}
	
	FILE* f;
	if ((f = fopen(argv[2], "r")) == NULL) {
		perror("trilaterator");
		return 1;
	}
	
	sphere beacons[4];
	for (nBeacons = 0; 
		fscanf(f, "%lf %lf %lf\n", &(beacons[nBeacons].x), &(beacons[nBeacons].y), &(beacons[nBeacons].z)) == 3 
		&& nBeacons < 4; nBeacons++);
	fclose(f);
	if (nBeacons < 3) {
		printf("Error reading input file. It must contain at least three instances of valid beacon locations.\n");
		return 1;
	}
	
	if ((semId = semget((key_t) SEMKEY, nBeacons, IPC_CREAT | 0666)) < 0 || 
		(shmId = shmget((key_t) SHMKEY, sizeof(double) * nBeacons, IPC_CREAT | 0666)) < 0
		 || (msgQueueId = msgget((key_t) MSGKEY, IPC_CREAT | 0666)) < 0) {
		perror("trilaterator");
		return -1;
	}
	int n;
	for (n = 0; n < nBeacons; n++) {
		semctl(semId, nBeacons, SETVAL, -1);
	}
	
	signal(SIGINT, quitSignalHandler);
	signal(SIGQUIT, quitSignalHandler);
	
	switch(option) {
		case 'l':
			locationCoordinator(beacons, TRUE);
			break;
		case 'e':
			locationCoordinator(beacons, FALSE);
			break;
	}
	
	return 0;
}

void locationCoordinator(sphere* beacons, bool lazy) {
	int n;
	char s[32];
	unsigned int beaconIds[nBeacons];
	struct sembuf sops[nBeacons + 1];
	for (n = 0; n < nBeacons; n++) {
		beaconIds[n] = n + 1;
		if (pthread_create(pIds + n, NULL, lazy ? lazyMeasure : eagerMeasure, (void*) (beaconIds + n))) {
			sprintf(s, "trilaterator thread %d", n);
			perror(s);
			kill(0, SIGKILL);
			break;
		}
		sops[n].sem_num = (unsigned short) n;
		sops[n].sem_op = -1;
		sops[n].sem_flg = 0;
	}
	sops[n].sem_num = (unsigned short) n;
	sops[n].sem_op = 4;
	sops[n].sem_flg = 0;
	
	double* sharedVar;
	double r[nBeacons];
	sphere result;
	while (TRUE) {
		semop(semId, sops, nBeacons);
		
		sharedVar = (double*) shmat(shmId, NULL, 0);
		memcpy(r, sharedVar, sizeof(double) * nBeacons);
		shmdt((void*) sharedVar);
		for (n = 0; n < nBeacons; n++) {
			beacons[n].r = r[n];
		}
		
		semop(semId, sops + nBeacons, 1);
		
		if (nBeacons > 3) {
			quatrilaterate(beacons, beacons + 1, beacons + 2, beacons + 3, &result);
		} else {
			trilaterate(beacons, beacons + 1, beacons + 2, &result);
		}
		printf("x = %.9f\ty = %.9f\tz = %.9f\n", result.x, result.y, result.z);
	}
}

void *lazyMeasure(void* bId) {
	unsigned int beaconId = *((unsigned int*) bId);
	queue q;
	initQueue(&q);
	message input;
	char s[32];
	double measure;
	struct sembuf sbArray;
	sbArray.sem_num = beaconId - 1;
	sbArray.sem_op = 1;
	sbArray.sem_flg = 0;
	struct sembuf sbMutex;
	sbMutex.sem_num = nBeacons;
	sbMutex.sem_op = -1;
	sbMutex.sem_flg = 0;
	double* sharedVar;
	
	while (TRUE) {
		if (msgrcv(msgQueueId, (message*) &input, sizeof(sphere), (long) beaconId, 0) < 0) {
			sprintf(s, "trilaterator thread %d", beaconId);
			perror(s);
			continue;
		}
		
		if (q.count >= QUEUE_SIZE) {
			dequeue(&q);
		}
		enqueue(&q, input.measureInfo.r);
		if (q.count == QUEUE_SIZE) {
			measure = filterConfidenceInterval(q.q, &(q.count));
			initQueue(&q);

			sharedVar = (double*) shmat(shmId, NULL, 0);
			*(sharedVar + beaconId - 1) = measure;
			shmdt((void*) sharedVar);
			semop(semId, &sbArray, 1);
			
			printf("Lazy measure %u: %.9f\n", beaconId, measure);
			
			semop(semId, &sbMutex, 1);
		}
	}
}

void *eagerMeasure(void* bId) {
	unsigned int beaconId = *((unsigned int*) bId);
	queue q;
	initQueue(&q);
	double qaux[QUEUE_SIZE];
	unsigned int qcount;
	message input;
	double measure;
	struct sembuf sbArray;
	sbArray.sem_num = beaconId - 1;
	sbArray.sem_op = 1;
	sbArray.sem_flg = 0;
	struct sembuf sbMutex;
	sbMutex.sem_num = nBeacons;
	sbMutex.sem_op = -1;
	sbMutex.sem_flg = 0;
	double* sharedVar;
	
	while (TRUE) {
		msgrcv(msgQueueId, (message*) &input, sizeof(sphere), (long) beaconId, 0);
		
		if (q.count >= QUEUE_SIZE) {
			dequeue(&q);
		}
		enqueue(&q, input.measureInfo.r);
		qcount = q.count;
		memcpy(qaux, q.q, qcount * sizeof(double));

		measure = filterConfidenceInterval(qaux, &qcount);
		
		sharedVar = (double*) shmat(shmId, NULL, 0);
		*(sharedVar + beaconId - 1) = measure;
		shmdt((void*) sharedVar);
		semop(semId, &sbArray, 1);

		printf("Eager measure %u: %.9f\n", beaconId, measure);
		
		semop(semId, &sbMutex, 1);
	}
}

void trilaterate(sphere* r1, sphere* r2, sphere* r3, sphere* result) {
	double d = r2->x, i = r3->x, j = r3->y;
	double *x, *y, *z;
	x = &(result->x);
	y = &(result->y);
	z = &(result->z);
	
	*x = (pow(d, 2.) + pow(r1->r, 2.) - pow(r2->r, 2.)) / (2. * d);
	*y = (pow(d, 2.) * i - d * (pow(i, 2.) + pow(j, 2.) + pow(r1->r, 2.) - pow(r3->r, 2.)) + i * (pow(r1->r, 2.) - pow(r2->r, 2.))) / (2. * d * j);
	*z = sqrt(pow(r1->r, 2.) - pow(*x, 2.) - pow(*y, 2.));
	if (errno == EDOM) {
		*z = -HUGE_VAL;
	}
}

void quatrilaterate(sphere* r1, sphere* r2, sphere* r3, sphere* r4, sphere* result) {
	double d = r2->x, i = r3->x, j = r3->y;
	double *x, *y, *z;
	x = &(result->x);
	y = &(result->y);
	z = &(result->z);
	
	*x = (pow(d, 2.) + pow(r1->r, 2.) - pow(r2->r, 2.)) / (2. * d);
	*y = (pow(d, 2.) * i - d * (pow(i, 2.) + pow(j, 2.) + pow(r1->r, 2.) - pow(r3->r, 2.)) + i * (pow(r1->r, 2.) - pow(r2->r, 2.))) / (2. * d * j);
	*z = sqrt(pow(r1->r, 2.) - pow(*x, 2.) - pow(*y, 2.));
	if (*z != 0) {
		double dist1 = sqrt(pow(r4->x - *x, 2.) + pow(r4->y - *y, 2.) + pow(r4->z - *z, 2.));
		double dist2 = sqrt(pow(r4->x - *x, 2.) + pow(r4->y - *y, 2.) + pow(r4->z + *z, 2.));
		if (dist2 < dist1) {
			*z = -(*z);
		}
	}
}

void quitSignalHandler(int signum) {
	printf("Shutting triangulator...\n");
	msgctl(msgQueueId, IPC_RMID, NULL);
	shmctl(shmId, IPC_RMID, NULL);
	int n;
	for (n = 0; n < nBeacons; n++) {
		pthread_cancel(pIds[n]);
	}
	struct timespec delay;
	delay.tv_sec = 0;
	delay.tv_nsec = 0.6 * nBeacons * pow(10., 9.);
	nanosleep(&delay, NULL);
	exit(EXIT_SUCCESS);
}
