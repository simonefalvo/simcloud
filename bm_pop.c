#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "basic.h"
#include "stats_utils.h"

int main()
{
    FILE *file_pop;
    FILE *file_int;
    char filename[32];
    char *fn = "data/n.dat";
    char *fn1 = "data/n1.dat";
    char *fn2 = "data/n2.dat";
    char *fnclet = "data/nclet.dat";
    char *fn1clet = "data/n1clet.dat";
    char *fn2clet = "data/n2clet.dat";
    char *fncloud = "data/ncloud.dat";
    char *fn1cloud = "data/n1cloud.dat";
    char *fn2cloud = "data/n2cloud.dat";
    char *fintperc = "data/intperc.dat";

    // output file descriptors
    int fd_n;
    int fd_n1;
    int fd_n2;
    int fd_nclet;
    int fd_n1clet;
    int fd_n2clet;
    int fd_ncloud;
    int fd_n1cloud;
    int fd_n2cloud;
    int fd_intperc;

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
    double nclet[K];
    double n1clet[K];
    double n2clet[K];
    double ncloud[K];
    double n1cloud[K];
    double n2cloud[K];
    double nsetup[K];
    double intpercent[K];

    struct confint_t c;

    // open output files
    fd_n = open(fn, O_WRONLY | O_CREAT, 00744);
    fd_n1 = open(fn1, O_WRONLY | O_CREAT, 00744);
    fd_n2 = open(fn2, O_WRONLY | O_CREAT, 00744);
    fd_nclet = open(fnclet, O_WRONLY | O_CREAT, 00744);
    fd_n1clet = open(fn1clet, O_WRONLY | O_CREAT, 00744);
    fd_n2clet = open(fn2clet, O_WRONLY | O_CREAT, 00744);
    fd_ncloud = open(fncloud, O_WRONLY | O_CREAT, 00744);
    fd_n1cloud = open(fn1cloud, O_WRONLY | O_CREAT, 00744);
    fd_n2cloud = open(fn2cloud, O_WRONLY | O_CREAT, 00744);
    fd_intperc = open(fintperc, O_WRONLY | O_CREAT, 00744);
    if (fd_intperc == -1 || fd_n == -1 || fd_n1 == -1 || fd_n2 == -1 || 
        fd_nclet == -1 || fd_n1clet == -1 || fd_n2clet == -1 ||
        fd_ncloud == -1 || fd_n1cloud == -1 || fd_n2cloud == -1)
        handle_error("opening an output file");

    for (r = 0; r < R; r++) {

        // init variables 
        memset(n, 0, K * sizeof(double));
        memset(n1, 0, K * sizeof(double));
        memset(n2, 0, K * sizeof(double));
        memset(n1clet, 0, K * sizeof(double));
        memset(n2clet, 0, K * sizeof(double));
        memset(nclet, 0, K * sizeof(double));
        memset(n1cloud, 0, K * sizeof(double));
        memset(n2cloud, 0, K * sizeof(double));
        memset(ncloud, 0, K * sizeof(double));
        memset(nsetup, 0, K * sizeof(double));
        memset(intpercent, 0, K * sizeof(double));

        // open input files
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
            nclet[i / b] += n1_clet + n2_clet;
            n1cloud[i / b] += n1_cloud;
            n2cloud[i / b] += n2_cloud;
            ncloud[i / b] += n1_cloud + n2_cloud;
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
            nclet[i] /= b;
            n1cloud[i] /= b;
            n2cloud[i] /= b;
            ncloud[i] /= b;
            nsetup[i] /= b;
            intpercent[i] /= b;
        }

        // print results 
        printf("\n  Replication %d results:\n", r);
        c = confint(n, K, ALPHA); 
        dprintf(fd_n, "%f %f\n", c.mean, c.w);
        printf("system mean population ......... = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n1, K, ALPHA); 
        dprintf(fd_n1, "%f %f\n", c.mean, c.w);
        printf("class 1 mean population ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2, K, ALPHA); 
        dprintf(fd_n2, "%f %f\n", c.mean, c.w);
        printf("class 2 mean population ........ = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n1clet, K, ALPHA); 
        dprintf(fd_n1clet, "%f %f\n", c.mean, c.w);
        printf("class 1 cloudlet mean population = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2clet, K, ALPHA); 
        dprintf(fd_n2clet, "%f %f\n", c.mean, c.w);
        printf("class 2 cloudlet mean population = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(nclet, K, ALPHA); 
        dprintf(fd_nclet, "%f %f\n", c.mean, c.w);
        printf("cloudlet mean population ....... = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n1cloud, K, ALPHA); 
        dprintf(fd_n1cloud, "%f %f\n", c.mean, c.w);
        printf("class 1 cloud mean population .. = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(n2cloud, K, ALPHA); 
        dprintf(fd_n2cloud, "%f %f\n", c.mean, c.w);
        printf("class 2 cloud mean population .. = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(ncloud, K, ALPHA); 
        dprintf(fd_ncloud, "%f %f\n", c.mean, c.w);
        printf("cloud mean population .......... = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(nsetup, K, ALPHA); 
        printf("setup mean population .......... = %lf  +/- %lf\n", c.mean, c.w);
        c = confint(intpercent, K, ALPHA); 
        dprintf(fd_intperc, "%f %f\n", c.mean, c.w);
        printf("interrupted job percantage ..... = %lf  +/- %lf\n", c.mean, c.w);
    }

    // close output file
    if (close(fd_n) == -1)
        handle_error("closing output file");
    if (close(fd_n1) == -1)
        handle_error("closing output file");
    if (close(fd_n2) == -1)
        handle_error("closing output file");
    if (close(fd_nclet) == -1)
        handle_error("closing output file");
    if (close(fd_n1clet) == -1)
        handle_error("closing output file");
    if (close(fd_n2clet) == -1)
        handle_error("closing output file");
    if (close(fd_ncloud) == -1)
        handle_error("closing output file");
    if (close(fd_n1cloud) == -1)
        handle_error("closing output file");
    if (close(fd_n2cloud) == -1)
        handle_error("closing output file");
    if (close(fd_intperc) == -1)
        handle_error("closing output file");

    return EXIT_SUCCESS;
}
