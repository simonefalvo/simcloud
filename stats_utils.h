#ifndef _STATS_UTILS_H
#define _STATS_UTILS_H

#include <math.h>
#include <stdlib.h>
#include "rvms.h"

struct confint_t {
	double mean;
	double w;
};

struct confint_t confint(double *batch_mean, long k, double alpha);
double autocor(double *data, size_t size, unsigned int lag);


#endif  /* _STATS_UTILS_H */
