#include <stdio.h>
#include <math.h>
#include <string.h>
#include "basic.h"
#include "rvms.h"

#define ALPHA   0.05
#define K       64 


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
    FILE *file_pop;
    FILE *file_int;
    char *filename;

    // temporary population variables
    double n1_clet;
    double n2_clet;
    double n1_cloud;
    double n2_cloud;
    double n_setup;
    double int_percent;

    long unsigned int i;
    unsigned int max_n;             // number of jobs to process
    unsigned long b;

    // population batch arrays
    double n[K];
    double n1[K];
    double n2[K];
    double n1clet[K];
    double n2clet[K];
    double n1cloud[K];
    double n2cloud[K];
    double nsetup[K];
    double intpercent[K];


    struct conf_int c;

    // init variables 
    memset(n, 0, K * sizeof(double));
    memset(n1, 0, K * sizeof(double));
    memset(n2, 0, K * sizeof(double));
    memset(n1clet, 0, K * sizeof(double));
    memset(n2clet, 0, K * sizeof(double));
    memset(n1cloud, 0, K * sizeof(double));
    memset(n2cloud, 0, K * sizeof(double));

    // open the files
    filename = "data/population_0.dat";
    file_pop = fopen(filename, "r");
    if (!file_pop)
        handle_error("opening data file");
    filename = "data/interruptions_0.dat";
    file_int = fopen(filename, "r");
    if (!file_int)
        handle_error("opening data file");

    // get batch size
    b = N_JOBS / K;
    max_n = b * K;

    // get data
    for (i = 0; i < max_n; i++) {
        if (fscanf(file_pop, "%lf %lf %lf %lf %lf\n",
            &n1_clet, &n2_clet, &n1_cloud, &n2_cloud, &n_setup) == EOF)
            handle_error("population file too short"); 
        if (fscanf(file_int, "%lf\n", &int_percent) == EOF)
            handle_error("interruptions file too short"); 

        n[i / b] += n1_clet + n2_clet + n1_cloud + n2_cloud + n_setup;
        n1[i / b] += n1_clet + n1_cloud;
        n2[i / b] += n2_clet + n2_cloud;
        n1clet[i / b] += n1_clet;
        n2clet[i / b] += n2_clet;
        n1cloud[i / b] += n1_cloud;
        n2cloud[i / b] += n2_cloud;
        nsetup[i / b] += n_setup;
        intpercent[i / b] += int_percent;
    }

    // close the files
    if (fclose(file_pop) == EOF)
        handle_error("closing data file");
    if (fclose(file_int) == EOF)
        handle_error("closing data file");

    // compute batch means
    for (i = 0; i < K; i++) {
        n[i] /= b;
        n1[i] /= b;
        n2[i] /= b;
        n1clet[i] /= b;
        n2clet[i] /= b;
        n1cloud[i] /= b;
        n2cloud[i] /= b;
        nsetup[i] /= b;
        intpercent[i] /= b;
    }

    // print results 
    c = confint(n, K, ALPHA); 
    printf("system mean population ......... = %lf  +/- %lf\n", c.sample_mean, c.w);
//    printf("autocorralation between batches: %lf\n", autocor(s, K, 1));
    c = confint(n1, K, ALPHA); 
    printf("class 1 mean population ........ = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(n2, K, ALPHA); 
    printf("class 2 mean population ........ = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(n1clet, K, ALPHA); 
    printf("class 1 cloudlet mean population = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(n2clet, K, ALPHA); 
    printf("class 2 cloudlet mean population = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(n1cloud, K, ALPHA); 
    printf("class 1 cloud mean population .. = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(n2cloud, K, ALPHA); 
    printf("class 2 cloud mean population .. = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(nsetup, K, ALPHA); 
    printf("setup mean population .......... = %lf  +/- %lf\n", c.sample_mean, c.w);
    c = confint(intpercent, K, ALPHA); 
    printf("interrupted job percantage ..... = %lf  +/- %lf\n", c.sample_mean, c.w);

    return EXIT_SUCCESS;
}
