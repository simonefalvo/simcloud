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
    FILE *file;

    // temporary service variables
    long unsigned int id;
    double s1_clet;
    double s2_clet;
    double s1_cloud;
    double s2_cloud;
    double setup;

    // completions
    unsigned long c1_clet;
    unsigned long c2_clet;
    unsigned long c1_cloud;
    unsigned long c2_cloud;
    unsigned long c_setup;

    // counters
    unsigned long n1 = 0;
    unsigned long n2 = 0;
    unsigned long n1_clet = 0;
    unsigned long n2_clet = 0;
    unsigned long n1_cloud = 0;
    unsigned long n2_cloud = 0;
    unsigned long n_int = 0;
    
    // batch sizes 
    unsigned long b;
    unsigned long b1;
    unsigned long b2;
    unsigned long b1_clet;
    unsigned long b2_clet;
    unsigned long b1_cloud;
    unsigned long b2_cloud;
    unsigned long b_int;

    long unsigned int i;
    unsigned int max_id;             // number of jobs to process

    // service batch arrays
    double s[K];
    double s1[K];
    double s2[K];
    double s1clet[K];
    double s2clet[K];
    double s1cloud[K];
    double s2cloud[K];
    double sint[K];


    struct conf_int c;

    // init structures
    memset(s, 0, K * sizeof(double));
    memset(s1, 0, K * sizeof(double));
    memset(s2, 0, K * sizeof(double));
    memset(s1clet, 0, K * sizeof(double));
    memset(s2clet, 0, K * sizeof(double));
    memset(s1cloud, 0, K * sizeof(double));
    memset(s2cloud, 0, K * sizeof(double));
    memset(sint, 0, K * sizeof(double));

    // open the file
    char *filename = "data/service_0_sorted.dat";
    file = fopen(filename, "r");
    if (!file)
        handle_error("opening data file");

    // get batch size
    if (fscanf(file, "-1 %ld %ld %ld %ld %ld\n",
        &c1_clet, &c2_clet, &c1_cloud, &c2_cloud, &c_setup) == EOF)
        handle_error("reading completions"); 
    b = (c1_clet + c2_clet + c1_cloud + c2_cloud) / K;
    b1 = (c1_clet + c1_cloud) / K;
    b2 = (c2_clet + c2_cloud) / K;
    b1_clet = c1_clet / K;
    b2_clet = c2_clet / K;
    b1_cloud = c1_cloud / K;
    b2_cloud = c2_cloud / K;
    b_int = c_setup / K;
    max_id = b * K;

    // get data
    while (id < max_id - 1) {
        if (fscanf(file, "%ld %lf %lf %lf %lf %lf\n", &id,
            &s1_clet, &s2_clet, &s1_cloud, &s2_cloud, &setup) == EOF)
            handle_error("file too short"); 
        //fprintf(stderr, "%ld: %ld\n", id, id / b);

        s[id / b] += s1_clet + s2_clet + s1_cloud + s2_cloud + setup;

        if (s1_clet || s1_cloud) {
            s1[n1 / b1] += s1_clet + s1_cloud;
            n1++;
        }
        if (s2_clet || s2_cloud) {
            s2[n2 / b2] += s2_cloud + s2_cloud;
            n2++;
        }
        if (s1_clet) {
            s1clet[n1_clet / b1_clet] += s1_clet;
            n1_clet++;
        }
        if (s2_clet && !setup) {
            s2clet[n2_clet / b2_clet] += s2_clet;
            n2_clet++;
        }
        if (s1_cloud) {
            s1cloud[n1_cloud / b1_cloud] += s1_cloud;
            n1_cloud++;
        }
        if (s2_cloud) {
            s2cloud[n2_cloud / b2_cloud] += s2_cloud;
            n2_cloud++;
        }
        if (setup) {
            sint[n_int / b_int] += s2_clet + s2_cloud + setup;
            n_int++;
        }
    }

    // close the file
    if (fclose(file) == EOF)
        handle_error("closing data file");

    // compute batch means
    for (i = 0; i < K; i++) {
        s[i] /= b;
        s1[i] /= b1;
        s2[i] /= b2;
        s1clet[i] /= b1_clet;
        s2clet[i] /= b2_clet;
        s1cloud[i] /= b1_cloud;
        s2cloud[i] /= b2_cloud;
        sint[i] /= b_int;
    }

    // print results 
    c = confint(s, K, ALPHA); 
    printf("system service time ......... = %f  +/- %f\n", c.sample_mean, c.w);
//    printf("autocorralation between batches: %f\n", autocor(s, K, 1));
    c = confint(s1, K, ALPHA); 
    printf("class 1 service time ........ = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(s2, K, ALPHA); 
    printf("class 2 service time ........ = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(s1clet, K, ALPHA); 
    printf("class 1 cloudlet service time = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(s2clet, K, ALPHA); 
    printf("class 2 cloudlet service time = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(s1cloud, K, ALPHA); 
    printf("class 1 cloud service time .. = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(s2cloud, K, ALPHA); 
    printf("class 2 cloud service time .. = %f  +/- %f\n", c.sample_mean, c.w);
    c = confint(sint, K, ALPHA); 
    printf("interrupted jobs service time = %f  +/- %f\n", c.sample_mean, c.w);

    return EXIT_SUCCESS;
}
