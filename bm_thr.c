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
    FILE *file;
    char filename[32];

    // output files names
    char *fx = "data/x.dat";
    char *fx1 = "data/x1.dat";
    char *fx2 = "data/x2.dat";
    char *fxclet = "data/xclet.dat";
    char *fx1clet = "data/x1clet.dat";
    char *fx2clet = "data/x2clet.dat";
    char *fxcloud = "data/xcloud.dat";
    char *fx1cloud = "data/x1cloud.dat";
    char *fx2cloud = "data/x2cloud.dat";

    // output file descriptors
    int fd_x;
    int fd_x1;
    int fd_x2;
    int fd_xclet;
    int fd_x1clet;
    int fd_x2clet;
    int fd_xcloud;
    int fd_x1cloud;
    int fd_x2cloud;


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
    double xclet[K];
    double x1clet[K];
    double x2clet[K];
    double xcloud[K];
    double x1cloud[K];
    double x2cloud[K];

    struct confint_t c;

    // opex output files
    fd_x = open(fx, O_WRONLY | O_CREAT, 00744);
    fd_x1 = open(fx1, O_WRONLY | O_CREAT, 00744);
    fd_x2 = open(fx2, O_WRONLY | O_CREAT, 00744);
    fd_xclet = open(fxclet, O_WRONLY | O_CREAT, 00744);
    fd_x1clet = open(fx1clet, O_WRONLY | O_CREAT, 00744);
    fd_x2clet = open(fx2clet, O_WRONLY | O_CREAT, 00744);
    fd_xcloud = open(fxcloud, O_WRONLY | O_CREAT, 00744);
    fd_x1cloud = open(fx1cloud, O_WRONLY | O_CREAT, 00744);
    fd_x2cloud = open(fx2cloud, O_WRONLY | O_CREAT, 00744);
    if (fd_x == -1 || fd_x1 == -1 || fd_x2 == -1 || 
        fd_xclet == -1 || fd_x1clet == -1 || fd_x2clet == -1 ||
        fd_xcloud == -1 || fd_x1cloud == -1 || fd_x2cloud == -1)
        handle_error("opening an output file");

    for (r = 0; r < R; r++) {

        // init structures
        memset(x, 0, K * sizeof(double));
        memset(x1, 0, K * sizeof(double));
        memset(x2, 0, K * sizeof(double));
        memset(xclet, 0, K * sizeof(double));
        memset(x1clet, 0, K * sizeof(double));
        memset(x2clet, 0, K * sizeof(double));
        memset(xcloud, 0, K * sizeof(double));
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
            xclet[i / b] += x1_clet + x2_clet;
            x1clet[i / b] += x1_clet;
            x2clet[i / b] += x2_clet;
            xcloud[i / b] += x1_cloud + x2_cloud;
            x1cloud[i / b] += x1_cloud;
            x2cloud[i / b] += x2_cloud;
        }

        // close the input file
        if (fclose(file) == EOF)
            handle_error("closing data file");

        // compute batch means
        for (i = 0; i < K; i++) {
            x[i] /= b;
            x1[i] /= b;
            x2[i] /= b;
            xclet[i] /= b;
            x1clet[i] /= b;
            x2clet[i] /= b;
            xcloud[i] /= b;
            x1cloud[i] /= b;
            x2cloud[i] /= b;
        }

        // print results 
        printf("\n  Replication %d results\n", r);

        c = confint(x, K, ALPHA); 
        dprintf(fd_x, "%f %f\n", c.mean, c.w);
        printf("system throughput ......... = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(x1, K, ALPHA); 
        dprintf(fd_x1, "%f %f\n", c.mean, c.w);
        printf("class 1 throughput ........ = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(x2, K, ALPHA); 
        dprintf(fd_x2, "%f %f\n", c.mean, c.w);
        printf("class 2 throughput ........ = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(xclet, K, ALPHA); 
        dprintf(fd_xclet, "%f %f\n", c.mean, c.w);
        printf("cloudlet throughput ....... = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(x1clet, K, ALPHA); 
        dprintf(fd_x1clet, "%f %f\n", c.mean, c.w);
        printf("class 1 cloudlet throughput = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(x2clet, K, ALPHA); 
        dprintf(fd_x2clet, "%f %f\n", c.mean, c.w);
        printf("class 2 cloudlet throughput = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(xcloud, K, ALPHA); 
        dprintf(fd_xcloud, "%f %f\n", c.mean, c.w);
        printf("cloud throughput .......... = %lf  +/- %lf\n", c.mean, c.w);
        
        c = confint(x1cloud, K, ALPHA); 
        dprintf(fd_x1cloud, "%f %f\n", c.mean, c.w);
        printf("class 1 cloud throughput .. = %lf  +/- %lf\n", c.mean, c.w);

        c = confint(x2cloud, K, ALPHA); 
        dprintf(fd_x2cloud, "%f %f\n", c.mean, c.w);
        printf("class 2 cloud throughput .. = %lf  +/- %lf\n", c.mean, c.w);
    }

    // close output file
    if (close(fd_x) == -1)
        handle_error("closing output file");
    if (close(fd_x1) == -1)
        handle_error("closing output file");
    if (close(fd_x2) == -1)
        handle_error("closing output file");
    if (close(fd_xclet) == -1)
        handle_error("closing output file");
    if (close(fd_x1clet) == -1)
        handle_error("closing output file");
    if (close(fd_x2clet) == -1)
        handle_error("closing output file");
    if (close(fd_xcloud) == -1)
        handle_error("closing output file");
    if (close(fd_x1cloud) == -1)
        handle_error("closing output file");
    if (close(fd_x2cloud) == -1)
        handle_error("closing output file");

    return EXIT_SUCCESS;
}
