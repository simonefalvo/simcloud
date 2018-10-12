#include <stdio.h>
#include <string.h>
#include "basic.h"
#include "stats_utils.h"

#define ALPHA   0.05
#define K       64 


int main()
{
    FILE *file;
    char filename[32];

    // temporary service variables
    double x1_clet;
    double x2_clet;
    double x1_cloud;
    double x2_cloud;

    long unsigned int i;
    unsigned int r;
    unsigned int max_n;             // number of jobs to process
    unsigned long b;

    // throughput batch arrays
    double x[K];
    double x1[K];
    double x2[K];
    double x1clet[K];
    double x2clet[K];
    double x1cloud[K];
    double x2cloud[K];


    struct conf_int c;

    for (r = 0; r < R; r++) {

        // init structures
        memset(x, 0, K * sizeof(double));
        memset(x1, 0, K * sizeof(double));
        memset(x2, 0, K * sizeof(double));
        memset(x1clet, 0, K * sizeof(double));
        memset(x2clet, 0, K * sizeof(double));
        memset(x1cloud, 0, K * sizeof(double));
        memset(x2cloud, 0, K * sizeof(double));

        // open the file
        sprintf(filename, "data/throughput_%d.dat", r);
        file = fopen(filename, "r");
        if (!file)
            handle_error("opening data file");

        // get batch size
        b = N_JOBS / K;
        max_n = b * K;

        // get data
        for (i = 0; i < max_n; i++) {
            if (fscanf(file, "%lf %lf %lf %lf\n",
                &x1_clet, &x2_clet, &x1_cloud, &x2_cloud) == EOF)
                handle_error("file too short"); 

            x[i / b] += x1_clet + x2_clet + x1_cloud + x2_cloud;
            x1[i / b] += x1_clet + x1_cloud;
            x2[i / b] += x2_clet + x2_cloud;
            x1clet[i / b] += x1_clet;
            x2clet[i / b] += x2_clet;
            x1cloud[i / b] += x1_cloud;
            x2cloud[i / b] += x2_cloud;
        }

        // close the file
        if (fclose(file) == EOF)
            handle_error("closing data file");

        // compute batch means
        for (i = 0; i < K; i++) {
            x[i] /= b;
            x1[i] /= b;
            x2[i] /= b;
            x1clet[i] /= b;
            x2clet[i] /= b;
            x1cloud[i] /= b;
            x2cloud[i] /= b;
        }

        // print results 
        printf("\n  Replication %d results\n", r);
        c = confint(x, K, ALPHA); 
        printf("system throughput ......... = %lf  +/- %lf\n", c.mean, c.w);
    //    printf("autocorralation between batches: %lf\n", autocor(s, K, 1));
        c = confint(x1, K, ALPHA); 
        printf("class 1 throughput ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(x2, K, ALPHA); 
        printf("class 2 throughput ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(x1clet, K, ALPHA); 
        printf("class 1 cloudlet throughput = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(x2clet, K, ALPHA); 
        printf("class 2 cloudlet throughput = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(x1cloud, K, ALPHA); 
        printf("class 1 cloud throughput .. = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(x2cloud, K, ALPHA); 
        printf("class 2 cloud throughput .. = %lf  +/- %lf\n", c.mean, c.w);
    }

    return EXIT_SUCCESS;
}
