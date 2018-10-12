#ifndef _STATS_UTILS_H
#define _STATS_UTILS_H

#include <math.h>
#include <stdlib.h>
#include "rvms.h"

struct conf_int {
	double mean;
	double w;
};

struct conf_int confint(double *batch_mean, long k, double alpha);
double autocor(double *data, size_t size, unsigned int lag);


#endif  /* _STATS_UTILS_H */
