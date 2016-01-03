#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "trilaterator.h"

#define MAX_RANGE 22.
#define FLUCTUATION_SPACE 1.
#define MEASURE_PRECISION 9
#define MEASURE_FREQUENCY 0.6

void measure();

int main(int argc, char* argv[]) {
	if (argc != 5) {
		printf("beacon <beaconId> <posX> <posY> <posZ>\n");
		return 1;
	}
	char inputError = 0;
	unsigned int id;
	double x, y, z;
	inputError = inputError || !sscanf(argv[1], "%u", &id);
	inputError = inputError || !sscanf(argv[2], "%lf", &x);
	inputError = inputError || !sscanf(argv[3], "%lf", &y);
	inputError = inputError || !sscanf(argv[4], "%lf", &z);
	if (inputError) {
		printf("Error in numerical input!!!\n");
		return -1;
	}
	
	int msgid;
	if ((msgid = msgget(MSGKEY, IPC_CREAT | 0666)) < 0) {
		char s[16];
		sprintf(s, "beacon %u", id);
		perror(s);
		return -1;
	}
	message template;
	template.beaconId = (long) id;
	template.measureInfo.x = x;
	template.measureInfo.y = y;
	template.measureInfo.z = z;
	
	measure(msgid, &template);
	
	return 0;
}

void measure(int msgQueue, message* msgTemplate) {
	srand(time(NULL));
	struct timespec delay;
	/*delay.tv_sec = 0;
	delay.tv_nsec = MEASURE_FREQUENCY * pow(10., 9.);*/
	delay.tv_sec = (time_t) (MEASURE_FREQUENCY * 10.);
	delay.tv_nsec = 0;
	
	double r = MAX_RANGE * pow(10., (double) MEASURE_PRECISION) * (double) rand() / (RAND_MAX + 1.0);
	r /= pow(10., (double) MEASURE_PRECISION);
	if ((int) (2. * rand() / (RAND_MAX + 1.0)) % 2) {
		r = -r;
	}

	char s[16];
	sprintf(s, "beacon %ld", msgTemplate->beaconId);
	double step;
	
	while (1) {
		printf("%s\t%.9f\n", s, r);
		msgTemplate->measureInfo.r = r;
		if (msgsnd(msgQueue, (message*) msgTemplate, sizeof(sphere), 0) < 0) {
			perror(s);
			continue;
		}
		nanosleep(&delay, NULL);
		step = FLUCTUATION_SPACE * pow(10., (double) MEASURE_PRECISION) * (double) rand() / (RAND_MAX + 1.0);
		step /= pow(10., (double) MEASURE_PRECISION);
		if ((int) (2. * rand() / (RAND_MAX + 1.0)) % 2) {
			step = -step;
		}
		r += step;
	}
}

