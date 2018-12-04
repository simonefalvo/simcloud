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

    // output file names
    char *fs = "data/s.dat";
    char *fs1 = "data/s1.dat";
    char *fs2 = "data/s2.dat";
    char *fsclet = "data/sclet.dat";
    char *fs1clet = "data/s1clet.dat";
    char *fs2clet = "data/s2clet.dat";
    char *fscloud = "data/scloud.dat";
    char *fs1cloud = "data/s1cloud.dat";
    char *fs2cloud = "data/s2cloud.dat";
    char *fsintr = "data/sintr.dat";

    // output file descriptors
    int fd_s;
    int fd_s1;
    int fd_s2;
    int fd_sclet;
    int fd_s1clet;
    int fd_s2clet;
    int fd_scloud;
    int fd_s1cloud;
    int fd_s2cloud;
    int fd_sintr;

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
    unsigned long n1;
    unsigned long n2;
    unsigned long n1_clet;
    unsigned long n2_clet;
    unsigned long n1_cloud;
    unsigned long n2_cloud;
    unsigned long n_intr;
    
    // batch sizes 
    unsigned long b;
    unsigned long b1;
    unsigned long b2;
    unsigned long b_clet;
    unsigned long b1_clet;
    unsigned long b2_clet;
    unsigned long b_cloud;
    unsigned long b1_cloud;
    unsigned long b2_cloud;
    unsigned long b_intr;

    long unsigned int i;
    unsigned int r;

    // service batch arrays
    double s[K];
    double s1[K];
    double s2[K];
    double sclet[K];
    double s1clet[K];
    double s2clet[K];
    double scloud[K];
    double s1cloud[K];
    double s2cloud[K];
    double sintr[K];

    struct confint_t c;

    // open output files
    fd_s = open(fs, O_WRONLY | O_CREAT, 00744);
    fd_s1 = open(fs1, O_WRONLY | O_CREAT, 00744);
    fd_s2 = open(fs2, O_WRONLY | O_CREAT, 00744);
    fd_sclet = open(fsclet, O_WRONLY | O_CREAT, 00744);
    fd_s1clet = open(fs1clet, O_WRONLY | O_CREAT, 00744);
    fd_s2clet = open(fs2clet, O_WRONLY | O_CREAT, 00744);
    fd_scloud = open(fscloud, O_WRONLY | O_CREAT, 00744);
    fd_s1cloud = open(fs1cloud, O_WRONLY | O_CREAT, 00744);
    fd_s2cloud = open(fs2cloud, O_WRONLY | O_CREAT, 00744);
    fd_sintr = open(fsintr, O_WRONLY | O_CREAT, 00744);
    if (fd_sintr == -1 || fd_s == -1 || fd_s1 == -1 || fd_s2 == -1 || 
        fd_sclet == -1 || fd_s1clet == -1 || fd_s2clet == -1 ||
        fd_scloud == -1 || fd_s1cloud == -1 || fd_s2cloud == -1)
        handle_error("opening an output file");

    for (r = 0; r < R; r++) {

        // init variables 
        memset(s, 0, K * sizeof(double));
        memset(s1, 0, K * sizeof(double));
        memset(s2, 0, K * sizeof(double));
        memset(sclet, 0, K * sizeof(double));
        memset(s1clet, 0, K * sizeof(double));
        memset(s2clet, 0, K * sizeof(double));
        memset(scloud, 0, K * sizeof(double));
        memset(s1cloud, 0, K * sizeof(double));
        memset(s2cloud, 0, K * sizeof(double));
        memset(sintr, 0, K * sizeof(double));
        n1 = 0;
        n2 = 0;
        n1_clet = 0;
        n2_clet = 0;
        n1_cloud = 0;
        n2_cloud = 0;
        n_intr = 0;

        // open the file
        sprintf(filename, "data/service_%d.dat", r);
        file = fopen(filename, "r");
        if (!file)
            handle_error("opening data file");

        // read completions
        if (fscanf(file, "-1 %ld %ld %ld %ld %ld\n",
            &c1_clet, &c2_clet, &c1_cloud, &c2_cloud, &c_setup) == EOF)
            handle_error("reading completions"); 

        // compute batch sizes
        b = (c1_clet + c2_clet + c1_cloud + c2_cloud) / K;
        b1 = (c1_clet + c1_cloud) / K;
        b2 = (c2_clet + c2_cloud) / K;
        b_clet = (c1_clet + c2_clet) / K;
        b1_clet = c1_clet / K;
        b2_clet = c2_clet / K;
        b_cloud = (c1_cloud + c2_cloud) / K;
        b1_cloud = c1_cloud / K;
        b2_cloud = c2_cloud / K;
        b_intr = c_setup / K;

        if (!b1_cloud) {
            fputs("\nWARNING: not enough class 1 jobs executed on the cloud. The value will not be reliable\n", stderr);
            b1_cloud = 1;
        }

        // get data
        while (fscanf(file, "%ld %lf %lf %lf %lf %lf\n", &id,
                &s1_clet, &s2_clet, &s1_cloud, &s2_cloud, &setup) != EOF) {

            s[id / b] += s1_clet + s2_clet + s1_cloud + s2_cloud + setup;

            if (s1_clet || s1_cloud) {
                s1[n1 / b1] += s1_clet + s1_cloud;
                n1++;
            }
            if (s2_clet || s2_cloud) {
                s2[n2 / b2] += s2_clet + s2_cloud + setup;
                n2++;
            }
            if (s1_clet) {
                s1clet[n1_clet / b1_clet] += s1_clet;
                sclet[(n1_clet + n2_clet) / b_clet] += s1_clet;
                n1_clet++;
            }
            if (s2_clet && !setup) {
                s2clet[n2_clet / b2_clet] += s2_clet;
                sclet[(n1_clet + n2_clet) / b_clet] += s2_clet;
                n2_clet++;
            }
            if (s1_cloud) {
                s1cloud[n1_cloud / b1_cloud] += s1_cloud;
                scloud[(n1_cloud + n2_cloud) / b_cloud] += s1_cloud;
                n1_cloud++;
            }
            if (s2_cloud) {
                s2cloud[n2_cloud / b2_cloud] += s2_cloud;
                scloud[(n1_cloud + n2_cloud) / b_cloud] += s2_cloud;
                n2_cloud++;
            }
            if (setup) {
                sintr[n_intr / b_intr] += s2_clet + s2_cloud + setup;
                n_intr++;
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
            sclet[i] /= b_clet;
            s1cloud[i] /= b1_cloud;
            s2cloud[i] /= b2_cloud;
            scloud[i] /= b_cloud;
            sintr[i] /= b_intr;
        }

        // print results 
        printf("\n  Replication %d results:\n", r);
        c = confint(s, K, ALPHA); 
        dprintf(fd_s, "%f %f\n", c.mean, c.w);
        printf("system service time ......... = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s, K, 1));
        c = confint(s1, K, ALPHA); 
        dprintf(fd_s1, "%f %f\n", c.mean, c.w);
        printf("class 1 service time ........ = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s1, K, 1));
        c = confint(s2, K, ALPHA); 
        dprintf(fd_s2, "%f %f\n", c.mean, c.w);
        printf("class 2 service time ........ = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s2, K, 1));
        c = confint(s1clet, K, ALPHA); 
        dprintf(fd_s1clet, "%f %f\n", c.mean, c.w);
        printf("class 1 cloudlet service time = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s1clet, K, 1));
        c = confint(s2clet, K, ALPHA); 
        dprintf(fd_s2clet, "%f %f\n", c.mean, c.w);
        printf("class 2 cloudlet service time = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s2clet, K, 1));
        c = confint(sclet, K, ALPHA); 
        dprintf(fd_sclet, "%f %f\n", c.mean, c.w);
        printf("cloudlet service time ....... = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(sclet, K, 1));
        c = confint(s1cloud, K, ALPHA); 
        dprintf(fd_s1cloud, "%f %f\n", c.mean, c.w);
        printf("class 1 cloud service time .. = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s1cloud, K, 1));
        c = confint(s2cloud, K, ALPHA); 
        dprintf(fd_s2cloud, "%f %f\n", c.mean, c.w);
        printf("class 2 cloud service time .. = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(s2cloud, K, 1));
        c = confint(scloud, K, ALPHA); 
        dprintf(fd_scloud, "%f %f\n", c.mean, c.w);
        printf("cloud service time .......... = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(scloud, K, 1));
        c = confint(sintr, K, ALPHA); 
        dprintf(fd_sintr, "%f %f\n", c.mean, c.w);
        printf("interrupted jobs service time = %f  +/- %f\n", c.mean, c.w);
        printf("autocorralation between batches: %f\n", autocor(sintr, K, 1));
    }

    // close output file
    if (close(fd_s) == -1)
        handle_error("closing output file");
    if (close(fd_s1) == -1)
        handle_error("closing output file");
    if (close(fd_s2) == -1)
        handle_error("closing output file");
    if (close(fd_sclet) == -1)
        handle_error("closing output file");
    if (close(fd_s1clet) == -1)
        handle_error("closing output file");
    if (close(fd_s2clet) == -1)
        handle_error("closing output file");
    if (close(fd_scloud) == -1)
        handle_error("closing output file");
    if (close(fd_s1cloud) == -1)
        handle_error("closing output file");
    if (close(fd_s2cloud) == -1)
        handle_error("closing output file");
    if (close(fd_sintr) == -1)
        handle_error("closing output file");

    return EXIT_SUCCESS;
}
