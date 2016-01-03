#ifndef KALMAN_H
#define KALMAN_H

double arithmeticalMean(double* population, unsigned int size);
double standardDeviation(double* population, unsigned int size);
double filterConfidenceInterval(double* population, unsigned int* size);

#endif

