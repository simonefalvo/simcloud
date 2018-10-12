#include <stdio.h>
#include <string.h>
#include "basic.h"
#include "stats_utils.h"

#define ALPHA   0.05
#define K       64 


int main()
{
    FILE *file_pop;
    FILE *file_int;
    char filename[32];

    // temporary population variables
    double n1_clet;
    double n2_clet;
    double n1_cloud;
    double n2_cloud;
    double n_setup;
    double int_percent;

    long unsigned int i;
    unsigned int r;
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

    for (r = 0; r < R; r++) {

        // init variables 
        memset(n, 0, K * sizeof(double));
        memset(n1, 0, K * sizeof(double));
        memset(n2, 0, K * sizeof(double));
        memset(n1clet, 0, K * sizeof(double));
        memset(n2clet, 0, K * sizeof(double));
        memset(n1cloud, 0, K * sizeof(double));
        memset(n2cloud, 0, K * sizeof(double));

        // open the files
        sprintf(filename, "data/population_%d.dat", r);
        file_pop = fopen(filename, "r");
        if (!file_pop)
            handle_error("opening data file");
        sprintf(filename, "data/interruptions_%d.dat", r);
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
        printf("\n  Replication %d results:\n", r);
        c = confint(n, K, ALPHA); 
        printf("system mean population ......... = %lf  +/- %lf\n", c.mean, c.w);
    //    printf("autocorralation between batches: %lf\n", autocor(s, K, 1));
        c = confint(n1, K, ALPHA); 
        printf("class 1 mean population ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2, K, ALPHA); 
        printf("class 2 mean population ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n1clet, K, ALPHA); 
        printf("class 1 cloudlet mean population = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2clet, K, ALPHA); 
        printf("class 2 cloudlet mean population = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n1cloud, K, ALPHA); 
        printf("class 1 cloud mean population .. = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2cloud, K, ALPHA); 
        printf("class 2 cloud mean population .. = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(nsetup, K, ALPHA); 
        printf("setup mean population .......... = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(intpercent, K, ALPHA); 
        printf("interrupted job percantage ..... = %lf  +/- %lf\n", c.mean, c.w);
    }

    return EXIT_SUCCESS;
}
