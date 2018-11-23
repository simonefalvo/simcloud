#include "stats_utils.h"

double autocor(double *data, size_t size, unsigned int j)
{
    double cosum_j = 0.0;
    double cosum_0 = 0.0;
    double mean = 0.0;
    unsigned int i;

/*  ONE PASS ALGORITHM (not working)
    for (i = 0; i < size - j; i++) {
        cosum_j += data[i] * data[i + j];
        cosum_0 += data[i] * data[i];
        mean += data[i];
    }
    for (i = size - j; i < size; i++) {
        cosum_0 += data[i] * data[i];
        mean += data[i];
    }
    mean /= size;
    cosum_j = (cosum_j / (size - j)) - (mean * mean) ;
    cosum_0 = (cosum_0 / size) - (mean * mean) ;
*/

    for (i = 0; i < size - j; i++)
        mean += data[i];
    mean /= size;
    for (i = 0; i < size - j; i++) {
        cosum_j += (data[i] - mean) * (data[i + j] - mean);
        cosum_0 += (data[i] - mean) * (data[i] - mean);
    }
    for (i = size - j; i < size; i++)
        cosum_0 += (data[i] - mean) * (data[i] - mean);
    cosum_j /= size - j;
    cosum_0 /= size;
    return cosum_j / cosum_0;
}


struct confint_t confint(double *sample, long size, double alpha)
{
	struct confint_t c;
	double sum = 0.0;
    double diff = 0.0;
    double mean = 0.0;
    double stdev = 0.0;
    double t;
	unsigned int i, j;
	
    // Welford's one pass Alg
	for (i = 0; i < size; i++) {
        j = i + 1;
        diff = sample[i] - mean;
        sum += diff * diff * (j - 1.0) / j;
        mean += diff / j;
    }
    stdev = sqrt(sum / size);

	t = idfStudent(size - 1, 1 - alpha/2);

	c.mean = mean;
	c.w = t * stdev / sqrt(size - 1);
    
	return c;  
}
