#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include "rvgs.h"
#include "rngs.h"
#include "basic.h"
#include "queue.h"
#include "eventq.h"
#include "dgen.h"


/* return the position of the event to be removed */
unsigned int rmpos(unsigned int n)
{
    unsigned int x;

    if (LDF_PLCY)
        x = n;
    else if (EDF_PLCY)
        x = 1;
    else { // random removal
        SelectStream(7);
        x = Uniform(1, n);
    }
    return x;
}


double srvjob(struct job_t job, unsigned int node,
                struct queue_t *queue, clock t)
{
    double service = GetService(job.class, node);
    struct event *e = alloc_event();
    memset(e, 0, sizeof(struct event));

    job.node = node;
    job.service[job.class + node] = service;
    e->job = job;
    e->time = t.current + service;
    e->type = E_DEPART;
    enqueue_event(e, queue);

    return service;
}


/* return the cloudlet execution time of the removed job */
double rplcjob(struct queue_t *queue, clock t, unsigned int n)
{
    double service;
    double left;                        // remaining service time
    double setup = GetService(J_CLASS2, SETUP);
    struct event *temp = alloc_event();
    struct job_t *job = &temp->job;
    struct event *e = NULL;
    
    job->class = J_CLASS2;
    job->node = CLET;
    temp->type = E_DEPART;
    e = remove_event(queue, temp, rmpos(n));
    
    left = e->time - t.current;     

    e->time = t.current + setup;
    e->type = E_SETUP;

    job = &e->job;
    job->service[J_CLASS2 + SETUP] = setup;
    service = job->service[J_CLASS2 + CLET];
    job->service[J_CLASS2 + CLET] -= left;

    enqueue_event(e, queue);
    free(temp);

    return service - left;
}


void audit(struct queue_t *queue, unsigned long *n)
{
    fprint_queue(stderr, queue, fprint_clet);
    fprintf(stderr, "cloudlet jobs: n1 = %ld, n2 = %ld\n\n", 
            n[J_CLASS1 + CLET], n[J_CLASS2 + CLET]);
    fprint_queue(stderr, queue, fprint_cloud);
    fprintf(stderr, "cloud jobs: n1 = %ld, n2 = %ld\n\n", 
            n[J_CLASS1 + CLOUD], n[J_CLASS2 + CLOUD]);
}



int main(void)
{
    clock t;                    /* simulation clock */
    unsigned int jobclass;      /* class of the current job */ 
    
    unsigned long n[5];         /* local population */

    unsigned long arrived;      /* global arrivals  */
    unsigned long a[5];         /* local arrivals   */
    unsigned long c[5];         /* local completions                    */
    unsigned long c_stop[5];    /* local completions in [START, STOP]   */ 
    unsigned long n_intr;       /* interruptions counter    */

    double s[5];                /* local service time   */
    double si_clet;             /* interrupted jobs cloudlet service time   */
    double si_cloud;            /* interrupted jobs cloud service time      */

    double area[5];             /* local area of jobs in time   */

    double t_start;             /* start time of the simulation     */ 
    double t_stop;              /* stop time of the simulation      */
    double elapsed;             /* elapsed time from the beginning  */

    long seed;

    unsigned int i;      // array index
    unsigned int r;      // replication index

    // output file descriptors
    int fd_srv;
    int fd_thr;
    int fd_pop;
    int fd_int;

    char outfile[32];
    char shell_cmd[128];
	
    struct event *e;
    struct queue_t queue;

    /* initialize event queue */
    queue.head = queue.tail = NULL;
    /* initialize simulation clock */
    t.current = START;

    printf("Enter initial seed: ");
    if (scanf("%ld", &seed) > 9)
        handle_error("bad input");
    

    for (r = 0; r < R; r++, GetSeed(&seed)) {

        // open output files
        sprintf(outfile, "data/srvtemp.dat");
        fd_srv = open(outfile, O_WRONLY | O_CREAT, 00744);
        sprintf(outfile, "data/throughput_%d.dat", r);
        fd_thr = open(outfile, O_WRONLY | O_CREAT, 00744);
        sprintf(outfile, "data/population_%d.dat", r);
        fd_pop = open(outfile, O_WRONLY | O_CREAT, 00744);
        sprintf(outfile, "data/interruptions_%d.dat", r);
        fd_int = open(outfile, O_WRONLY | O_CREAT, 00744);
        if (fd_srv == -1 || fd_thr == -1 || 
            fd_pop == -1 || fd_int == -1)
            handle_error("opening output file");

        // init variables 
        for (i = 0; i < 5; i++) {
            n[i] = 0;       
            a[i] = 0;   
            c[i] = 0;       
            c_stop[i] = 0;  
            s[i] = 0.0; 
            area[i] = 0.0;
        }
        n_intr = 0;            
        arrived = 0;         
        si_clet = 0.0;           
        si_cloud = 0.0;          
        t_start = t.current;
        t_stop = INFINITY;


        PlantSeeds(seed);
        e = alloc_event();
        memset(e, 0, sizeof(struct event));
        e->time = GetArrival(&jobclass);
        e->type = E_ARRIVL;
        e->job.class = jobclass;

        enqueue_event(e, &queue);


        while (queue.head != NULL) {

            e = dequeue_event(&queue);
            t.next = e->time;                               /* next event time */
            
            for (i = 0; i < 5; i++)                         /* update integral  */
                area[i] += (t.next - t.current) * n[i];

            t.current = t.next;                             /* advance the clock */


            switch (e->type) {

            case E_ARRIVL:                                  /* process an arrival */

                e->job.id = arrived;
                arrived++;

                if (e->job.class == J_CLASS1) {            /* process a class 1 arrival */
                    if (n[J_CLASS1 + CLET] == N) {
                        srvjob(e->job, CLOUD, &queue, t);
                        a[J_CLASS1 + CLOUD]++;
                        n[J_CLASS1 + CLOUD]++;
                    }
                    else if (n[J_CLASS1 + CLET] + n[J_CLASS2 + CLET] < S) { // accept job
                            srvjob(e->job, CLET, &queue, t);
                            a[J_CLASS1 + CLET]++;
                            n[J_CLASS1 + CLET]++;
                        }
                        else if (n[J_CLASS2 + CLET] > 0) { // replace a class 2 job 
                                si_clet += rplcjob(&queue, t, n[J_CLASS2 + CLET]);
                                srvjob(e->job, CLET, &queue, t);
                                a[J_CLASS1 + CLET]++;
                                n[J_CLASS1 + CLET]++;
                                n[J_CLASS2 + CLET]--;
                                a[J_CLASS2 + SETUP]++;
                                n[J_CLASS2 + SETUP]++;
                                n_intr++;
                            }
                            else { // accept job
                                srvjob(e->job, CLET, &queue, t);
                                a[J_CLASS1 + CLET]++;
                                n[J_CLASS1 + CLET]++;
                            }
                }
                else {                 /* process a class 2 arrival */
                    if (n[J_CLASS1 + CLET] + n[J_CLASS2 + CLET] >= S) {
                        srvjob(e->job, CLOUD, &queue, t);
                        a[J_CLASS2 + CLOUD]++;
                        n[J_CLASS2 + CLOUD]++;
                    }
                    else {
                        srvjob(e->job, CLET, &queue, t);
                        a[J_CLASS2 + CLET]++;
                        n[J_CLASS2 + CLET]++;
                    }
                }
                e->time = GetArrival(&jobclass);       /* set next arrival time */
                e->job.class = jobclass;               /* set next arrival class */
                
                if (arrived >= N_JOBS) {
                    e->type = E_IGNRVL; // arrival to not process
                    t_stop = t.current; 
                }
                enqueue_event(e, &queue);

                break;

           case E_IGNRVL:
                if (n[J_CLASS1 + CLET] + n[J_CLASS2 + CLET] + 
                    n[J_CLASS1 + CLOUD] + n[J_CLASS2 + CLOUD] + 
                    n[J_CLASS2 + SETUP]) {      // there are jobs to be processed
                    e->time = GetArrival(&jobclass); /* set next arrival to ignore */
                    enqueue_event(e, &queue);
                }
                else free(e);
                break;

           case E_SETUP:
                si_cloud += srvjob(e->job, CLOUD, &queue, t);
                a[J_CLASS2 + CLOUD]++;
                n[J_CLASS2 + CLOUD]++;
                n[J_CLASS2 + SETUP]--;
                c[J_CLASS2 + SETUP]++;
                break;

            case E_DEPART:                /* process a departure */
                n[e->job.class + e->job.node]--;
                c[e->job.class + e->job.node]++;
                if (t.current <= t_stop)
                    c_stop[e->job.class + e->job.node]++;

                // populate s to print results to stdout
                if (DISPLAY) {
                    for (i = 0; i < 5; i++) 
                        s[i] += e->job.service[i];
                }

                // write data to outfile
                if (OUTPUT) {
                    dprintf(fd_srv, "%ld %f %f %f %f %f\n", e->job.id, 
                        e->job.service[J_CLASS1 + CLET], 
                        e->job.service[J_CLASS2 + CLET], 
                        e->job.service[J_CLASS1 + CLOUD], 
                        e->job.service[J_CLASS2 + CLOUD],
                        e->job.service[J_CLASS2 + SETUP]);
                    elapsed = t.current - t_start;
                    dprintf(fd_thr, "%f %f %f %f\n", 
                        c[J_CLASS1 + CLET] / elapsed, 
                        c[J_CLASS2 + CLET] / elapsed, 
                        c[J_CLASS1 + CLOUD] / elapsed,
                        c[J_CLASS2 + CLOUD] / elapsed);
                    dprintf(fd_pop, "%f %f %f %f %f\n", 
                        area[J_CLASS1 + CLET] / elapsed, 
                        area[J_CLASS2 + CLET] / elapsed, 
                        area[J_CLASS1 + CLOUD] / elapsed,
                        area[J_CLASS2 + CLOUD] / elapsed, 
                        area[J_CLASS2 + SETUP] / elapsed);

                    unsigned long aj2 = a[J_CLASS2 + CLET] + 
                                        a[J_CLASS2 + CLOUD] - n_intr;
                    if (!aj2)
                        dprintf(fd_int, "%f\n", 0.0);
                    else 
                        dprintf(fd_int, "%f\n", (double) n_intr / aj2);
                }
                    
                free(e);
                break;
            
            default:
                handle_error("unknown event type");
            }

        }

        // write completions
        if (OUTPUT) {
            dprintf(fd_srv, "%d %ld %ld %ld %ld %ld\n", -1,
                    c[J_CLASS1 + CLET], c[J_CLASS2 + CLET], 
                    c[J_CLASS1 + CLOUD], c[J_CLASS2 + CLOUD],
                    c[J_CLASS2 + SETUP]);
        }

        // close output files
        if (close(fd_srv) == -1)
            handle_error("closing output file");
        if (close(fd_thr) == -1)
            handle_error("closing output file");
        if (close(fd_pop) == -1)
            handle_error("closing output file");
        if (close(fd_int) == -1)
            handle_error("closing output file");
        
        // sort service output data 
        sprintf(shell_cmd, "sort -n data/srvtemp.dat > data/service_%d.dat; "
                           "rm data/srvtemp.dat", r);
        if (system(shell_cmd) == -1)
            handle_error("system() - executing sort command");



        /************************** print results *****************************/

        if (DISPLAY) {
            double service = 0.0;           /* total service      */
            unsigned long completions = 0;  /* total completions  */
            unsigned long compl_stop = 0;   /* total completions in [START, STOP] */
            double area_tot = 0.0;          /* time integrated # in the system */

            for (i = 0; i < 5; i++) {
                completions += c[i];
                compl_stop += c_stop[i];
                area_tot += area[i];
                service += s[i];
            }
            

            printf("\nSimulation number %d run with N=%d, S=%d, t_start=%.2f, "
                   "t_stop=%.2f\n", r, N, S, t_start, t_stop);

            printf("\nRATES\n");
            printf("  System arrival rate ................ = %f\n", 
                arrived / (t_stop - t_start));
            printf("  System departure rate .............. = %f\n", 
                compl_stop / (t_stop - t_start));
            printf("  Cloudlet class 1 Arrival rate ...... = %f\n", 
                a[J_CLASS1 + CLET] / (t_stop - t_start));
            printf("  Cloudlet class 2 Arrival rate ...... = %f\n", 
                a[J_CLASS2 + CLET] / (t_stop - t_start));
            printf("  Cloudlet arrival rate .............. = %f\n", 
                (a[J_CLASS1 + CLET] + a[J_CLASS2 + CLET]) 
                / (t_stop - t_start));
            printf("  Cloud class 1 Arrival rate ......... = %f\n", 
                a[J_CLASS1 + CLOUD] / (t_stop - t_start));
            printf("  Cloud class 2 Arrival rate ......... = %f\n", 
                a[J_CLASS2 + CLOUD] / (t_stop - t_start));
            printf("  Cloud class 2 direct arrival rate .. = %f\n", 
                (a[J_CLASS2 + CLOUD] - n_intr) / (t_stop - t_start));
            printf("  Cloud Arrival rate ................. = %f\n", 
                (a[J_CLASS1 + CLOUD] + a[J_CLASS2 + CLOUD]) 
                / (t_stop - t_start));


            printf("\nRESPONSE TIME\n");
            printf("  Class 1 cloudlet response time ..... = %f (%.2f)\n", 
                s[J_CLASS1 + CLET] / c[J_CLASS1 + CLET], 1 / M1CLET);
            printf("  Class 2 cloudlet response time ..... = %f (%.2f)\n", 
                (s[J_CLASS2 + CLET] - si_clet) / c[J_CLASS2 + CLET], 1 / M2CLET);
            printf("  Cloudlet response time ............. = %f\n", 
                (s[J_CLASS1 + CLET] + s[J_CLASS2 + CLET]) / 
                (c[J_CLASS1 + CLET] + c[J_CLASS2 + CLET]));
            printf("  Class 1 cloud response time ........ = %f (%.2f)\n", 
                s[J_CLASS1 + CLOUD] / c[J_CLASS1 + CLOUD], 1 / M1CLOUD);
            printf("  Class 2 cloud response time ........ = %f (%.2f)\n", 
                s[J_CLASS2 + CLOUD] / c[J_CLASS2 + CLOUD], 1 / M2CLOUD);
            printf("  Cloud response time ................ = %f\n", 
                (s[J_CLASS1 + CLOUD] + s[J_CLASS2 + CLOUD]) / 
                (c[J_CLASS1 + CLOUD] + c[J_CLASS2 + CLOUD]));
            printf("  Class 1 system response time ....... = %f\n", 
                (s[J_CLASS1 + CLET] + s[J_CLASS1 + CLOUD]) / 
                (c[J_CLASS1 + CLET] + c[J_CLASS1 + CLOUD]));
            printf("  Class 2 system response time ....... = %f\n", 
                (s[J_CLASS2 + CLET] + s[J_CLASS2 + CLOUD] + s[J_CLASS2 + SETUP]) / 
                (c[J_CLASS2 + CLET] + c[J_CLASS2 + CLOUD]));
            printf("  Setup time ......................... = %f (%.2f)\n", 
                s[J_CLASS2 + SETUP] / n_intr, 1 / MSETUP);
            printf("  Interrupted tasks response time .... = %f\n", 
                (si_clet + s[J_CLASS2 + SETUP] + si_cloud) / n_intr);
            printf("  Global system response time ........ = %f\n", 
                service / arrived);


            printf("\nPOPULATION\n");
            printf("  Class 1 cloudlet mean population ... = %f\n", 
                area[J_CLASS1 + CLET] / (t.current - t_start));
            printf("  Class 2 cloudlet mean population ... = %f\n", 
                area[J_CLASS2 + CLET] / (t.current - t_start));
            printf("  Cloudlet mean population ........... = %f\n", 
                (area[J_CLASS1 + CLET] + area[J_CLASS2 + CLET]) 
                / (t.current - t_start));
            printf("  Class 1 cloud mean population ...... = %f\n", 
                area[J_CLASS1 + CLOUD] / (t.current - t_start));
            printf("  Class 2 cloud mean population ...... = %f\n", 
                area[J_CLASS2 + CLOUD] / (t.current - t_start));
            printf("  Cloud mean population .............. = %f\n", 
                (area[J_CLASS1 + CLOUD] + area[J_CLASS2 + CLOUD]) 
                / (t.current - t_start));
            printf("  System mean population ............. = %f\n",
                area_tot / (t.current - t_start));
            printf("  Little's Law: arrival rate = N/S ... = %f (%.2f)\n", 
                (area_tot / (t.current - t_start)) 
                / (service / arrived), L1 + L2);


            printf("\nTHROUGHPUT\n");
            printf("  Class 1 cloudlet throughput ........ = %f\n", 
                c_stop[J_CLASS1 + CLET] / (t_stop - t_start));
            printf("  Class 2 cloudlet throughput ........ = %f\n", 
                c_stop[J_CLASS2 + CLET] / (t_stop - t_start));
            printf("  Cloudlet throughput ................ = %f\n", 
                (c_stop[J_CLASS1 + CLET] + c_stop[J_CLASS2 + CLET]) 
                / (t_stop - t_start));
            printf("  Class 1 cloud throughput ........... = %f\n", 
                c_stop[J_CLASS1 + CLOUD] / (t_stop - t_start));
            printf("  Class 2 cloud throughput ........... = %f\n", 
                c_stop[J_CLASS2 + CLOUD] / (t_stop - t_start));
            printf("  Cloud throughput ................... = %f\n", 
                (c_stop[J_CLASS1 + CLOUD] + c_stop[J_CLASS2 + CLOUD]) 
                / (t_stop - t_start));
            printf("  Class 1 system throughput .......... = %f\n", 
                (c_stop[J_CLASS1 + CLET] + c_stop[J_CLASS1 + CLOUD]) 
                / (t_stop - t_start));
            printf("  Class 2 system throughput .......... = %f\n",  
                (c_stop[J_CLASS2 + CLET] + c_stop[J_CLASS2 + CLOUD]) 
                / (t_stop - t_start));
            printf("  Global system throughput ........... = %f\n", 
                compl_stop / (t_stop - t_start));
            

            printf("\nROUTING\n");
            printf("  Total arrived jobs ................. = %9ld\n", arrived);
            printf("  Total processed jobs ............... = %9ld\n", completions);
            printf("  Class 1 cloudlet arrivals .......... = %9ld (%4.2f %%)\n", 
                a[J_CLASS1 + CLET], (double) a[J_CLASS1 + CLET] / arrived * 100);
            printf("  Class 1 cloudlet completions ....... = %9ld (%4.2f %%)\n", 
                c[J_CLASS1 + CLET], (double) c[J_CLASS1 + CLET] / arrived * 100);
            printf("  Class 2 cloudlet arrivals .......... = %9ld (%4.2f %%)\n", 
                a[J_CLASS2 + CLET], (double) a[J_CLASS2 + CLET] / arrived * 100); 
            printf("  Class 2 cloudlet completions ....... = %9ld (%4.2f %%)\n", 
                c[J_CLASS2 + CLET], (double) c[J_CLASS2 + CLET] / arrived * 100);
            printf("  Class 1 cloud arrivals ............. = %9ld (%4.2f %%)\n", 
                a[J_CLASS1 + CLOUD], (double) a[J_CLASS1 + CLOUD] / arrived * 100);
            printf("  Class 1 cloud completions .......... = %9ld (%4.2f %%)\n", 
                c[J_CLASS1 + CLOUD], (double) c[J_CLASS1 + CLOUD] / arrived * 100);
            printf("  Class 2 cloud arrivals ............. = %9ld (%4.2f %%)\n", 
                a[J_CLASS2 + CLOUD], (double) (a[J_CLASS2 + CLOUD] - n_intr) / arrived * 100);
            printf("  Class 2 cloud completions .......... = %9ld (%4.2f %%)\n", 
                c[J_CLASS2 + CLOUD], (double) (c[J_CLASS2 + CLOUD] - n_intr) / arrived * 100);
            printf("  Interrupted jobs ................... = %9ld (%4.2f %%)\n",
                n_intr, (double) n_intr / arrived * 100);
            printf("  Class 2 Interrupted jobs percentage  = %9.4f %%\n",
                (double) n_intr / 
                (c[J_CLASS2 + CLET] + c[J_CLASS2 + CLOUD]) * 100);
            printf("  Cloudlet interrupted jobs percentage = %9.4f %%\n", (double) 
                n_intr / a[J_CLASS2 + CLET] * 100);
        }
    }

    return EXIT_SUCCESS;
}
