#include <math.h>

#include "kalman.h"

double arithmeticalMean(double* population, unsigned int size) {
	double sum = 0.;
	unsigned int n;
	for (n = 0; n < size; n++) {
		sum += population[n];
	}
	return sum / (double) size;
}

double standardDeviation(double* population, unsigned int size) {
	double mean = arithmeticalMean(population, size);
	double sum = 0.;
	unsigned int n;
	for (n = 0; n < size; n++) {
		sum += pow(population[n] - mean, 2.);
	}
	return sqrt(sum / (double) size);
}

double filterConfidenceInterval(double* population, unsigned int* size) {
	double mean = arithmeticalMean(population, *size);
	double deviation = standardDeviation(population, *size);
	unsigned int filteredSize = 0, n;
	for (n = 0; n < *size; n++) {
		if (population[n] >= mean - deviation || population[n] <= mean + deviation) {
			population[filteredSize++] = population[n];
		}
	}
	*size = filteredSize;
	
	return arithmeticalMean(population, filteredSize);
}

