#include <stdio.h>
#include <string.h>
#include "basic.h"
#include "stats_utils.h"

#define K       64 
#define ALPHA   0.05


int main()
{
    FILE *file;
    char filename[32];

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
    unsigned long n_int;
    
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
    unsigned long b_int;

    long unsigned int i;
    unsigned int r;
    unsigned int max_id;             // number of jobs to process

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
    double sint[K];


    struct conf_int c;

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
        memset(sint, 0, K * sizeof(double));
        n1 = 0;
        n2 = 0;
        n1_clet = 0;
        n2_clet = 0;
        n1_cloud = 0;
        n2_cloud = 0;
        n_int = 0;

        // open the file
        sprintf(filename, "data/service_%d.dat", r);
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
        b_clet = (c1_clet + c2_clet) / K;
        b1_clet = c1_clet / K;
        b2_clet = c2_clet / K;
        b_cloud = (c1_cloud + c2_cloud) / K;
        b1_cloud = c1_cloud / K;
        b2_cloud = c2_cloud / K;
        b_int = c_setup / K;
        max_id = b * K;

        id = 0;
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
            sclet[i] /= b_clet;
            s1cloud[i] /= b1_cloud;
            s2cloud[i] /= b2_cloud;
            scloud[i] /= b_cloud;
            sint[i] /= b_int;
        }

        // print results 
        printf("\n  Replication %d results:\n", r);
        c = confint(s, K, ALPHA); 
        printf("system service time ......... = %f  +/- %f\n", c.mean, c.w);
    //    printf("autocorralation between batches: %f\n", autocor(s, K, 1));
        c = confint(s1, K, ALPHA); 
        printf("class 1 service time ........ = %f  +/- %f\n", c.mean, c.w);
        c = confint(s2, K, ALPHA); 
        printf("class 2 service time ........ = %f  +/- %f\n", c.mean, c.w);
        c = confint(s1clet, K, ALPHA); 
        printf("class 1 cloudlet service time = %f  +/- %f\n", c.mean, c.w);
        c = confint(s2clet, K, ALPHA); 
        printf("class 2 cloudlet service time = %f  +/- %f\n", c.mean, c.w);
        c = confint(sclet, K, ALPHA); 
        printf("cloudlet service time ....... = %f  +/- %f\n", c.mean, c.w);
        c = confint(s1cloud, K, ALPHA); 
        printf("class 1 cloud service time .. = %f  +/- %f\n", c.mean, c.w);
        c = confint(s2cloud, K, ALPHA); 
        printf("class 2 cloud service time .. = %f  +/- %f\n", c.mean, c.w);
        c = confint(scloud, K, ALPHA); 
        printf("cloud service time .......... = %f  +/- %f\n", c.mean, c.w);
        c = confint(sint, K, ALPHA); 
        printf("interrupted jobs service time = %f  +/- %f\n", c.mean, c.w);
    }

    return EXIT_SUCCESS;
}
