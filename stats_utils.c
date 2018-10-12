#include "stats_utils.h"


double autocor(double *data, size_t size, unsigned int lag)
{
    double sum = 0.0;
    double cosum = 0.0;
    double cosum_0 = 0.0;
    double mean;
    unsigned int i;

    for (i = 0; i < size - lag; i++) {
        cosum += data[i] * data[i + lag];
        cosum_0 += data[i] * data[i];
        sum += data[i];
    }
    for (i = size - lag; i < size; i++) {
        cosum_0 += data[i] * data[i];
        sum += data[i];
    }
    mean = sum / size;
    cosum = (cosum / (size - lag)) - (mean * mean);
    cosum_0 = (cosum_0 / size) - (mean * mean);

    return cosum / cosum_0;
}


struct conf_int confint(double *sample, long k, double alpha){

	struct conf_int c;
	double mean = 0.0;
    double stdev = 0.0;
    double t;
	unsigned int i;
	
	for(i = 0; i < k; i++) 
    	mean += sample[i];
    mean /= k;	
    
    for(i = 0; i < k; i++)
		stdev += pow(sample[i] - mean, 2);
	stdev = sqrt(stdev /k);

	t = idfStudent(k - 1, 1 - alpha/2);

	c.mean = mean;
	c.w = t * stdev / sqrt(k - 1);
    
	return c;  
}
