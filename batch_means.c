#include <stdio.h>
#include <math.h>
#include <string.h>
#include "basic.h"
#include "rvms.h"


struct conf_int {
	double w;
	double sample_mean;
};


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


struct conf_int confint(double *batch_mean, long k, double alpha){

	struct conf_int c_int;
	double sample_mean = 0.0, dev_std = 0.0;
    double t, w;
	unsigned int i;
	
	for(i = 0; i < k; i++) 
    	sample_mean += batch_mean[i];
    sample_mean = sample_mean / k;	
    
    for(i = 0; i < k; i++)
		dev_std += pow(batch_mean[i] - sample_mean, 2);
	dev_std = sqrt(dev_std/k);

	t = idfStudent(k - 1, 1 - alpha/2);
	w = (t * dev_std) / sqrt(k - 1);
    c_int.w = w;
	c_int.sample_mean = sample_mean;
    
	return c_int;  
}


int main()
{
    FILE *file;

    // temporary variables
    long unsigned int id = 0;
    double s1_clet = 0.0;
    double s2_clet = 0.0;
    double s1_cloud = 0.0;
    double s2_cloud = 0.0;
    double setup = 0.0;

    
    long unsigned int i;
    long unsigned int n_job = 100000;
    double alpha = 0.05;
    unsigned int k = 64;
    unsigned int b = n_job / k;
    unsigned int n = k * b;             // number of jobs to process
    double s[k];


    struct conf_int c;

    // init structures
    memset(s, 0, k * sizeof(double));

    // open the file
    char *filename = "data/service_0_sorted.dat";
    file = fopen(filename, "r");
    if (!file)
        handle_error("opening data file");
    
    // get data
    while (id < n - 1) {
        if (fscanf(file, "%ld %lf %lf %lf %lf %lf\n", &id,
            &s1_clet, &s2_clet, &s1_cloud, &s2_cloud, &setup) == EOF)
            handle_error("file too short"); 
        //fprintf(stderr, "%ld: %ld\n", id, id / b);
        s[id / b] += s1_clet + s2_clet + s1_cloud + s2_cloud + setup;
    }

    // close the file
    if (fclose(file) == EOF)
        handle_error("closing data file");

    // compute batch means
    for (i = 0; i < k; i++)
        s[i] = s[i] / b;

    c = confint(s, k, alpha); 
    printf("system service time: %f  +/- %f\n", c.sample_mean, c.w);
    printf("autocorralation between batchs: %f\n", autocor(s, k, 1));

    return EXIT_SUCCESS;
}
