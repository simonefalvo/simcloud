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
#include "rvms.h"


double GetArrival(unsigned int *j)
{
	const double mean[2] = {1/L1, 1/L2};	
	static double arrival[2] = {START, START};
	static int init = 1;
	double temp;
	
	if (init) {
		SelectStream(0);
		arrival[0] += Exponential(mean[0]);
		SelectStream(1);
		arrival[1] += Exponential(mean[1]);
		init=0;
	}

	if (arrival[0] <= arrival[1])
		*j = 0;
	else
		*j = 1;

	temp = arrival[*j];
	SelectStream(*j);
	arrival[*j] += Exponential(mean[*j]);

	return temp;
}



double GetService(int j, int n)
{
	const double mean[4] = {1/M1CLET, 1/M2CLET,
                            1/M1CLOUD, 1/M2CLOUD};	
    SelectStream(j + n + 2);
    return Exponential(mean[j + n]);
}



double GetSetup()
{
    SelectStream(6);
    return Exponential(SETUP);
}

unsigned int rmpos(unsigned int n)
{
    unsigned int x;

    if (YNGER_PLCY)
        x = n;
    else if (OLDER_PLCY)
        x = 1;
    else {
        SelectStream(7);
        x = Uniform(1, n);
    }
    //fprintf(stderr, "n2=%d, pos=%d\n", n, x);
    return x;
}


double srvjob(struct job_t job, unsigned int node, struct queue_t *queue, clock t)
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
    double setup = GetSetup();
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
    job->setup = setup;
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
    
    unsigned long n[4];         /* local population */
    unsigned long n_setup;      /* setup population */

    unsigned long arrived;      /* global arrivals  */
    unsigned long a[4];         /* local arrivals   */
    unsigned long c[4];         /* local completions                    */
    unsigned long c_stop[4];    /* local completions in [START, STOP]   */ 
    unsigned long n_int;        /* total interrupted jobs   */

    double s[4];                /* local service time   */
    double si_clet;     	    /* interrupted jobs cloudlet service time   */
    double si_cloud;    	    /* interrupted jobs cloud service time      */
    double s_setup;     	    /* interruptes jobs setup time              */

    double area[4];             /* local area of jobs in time */
    double area_setup;          /* area of setup jobs in time */

	double t_start;     /* start time of the simulation                         */ 
	double t_stop;      /* stop time of the simulation                          */
	double elapsed;     /* elapsed time from the beginning of the simulation    */

    long seed;

    unsigned int i;      // array index
    unsigned int r;      // replication index
    // char *node;

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
        for (i = 0; i < 4; i++) {
            n[i] = 0;       
            a[i] = 0;   
            c[i] = 0;       
            c_stop[i] = 0;  
            s[i] = 0.0; 
            area[i] = 0.0;
        }
        n_setup = 0;           
        n_int = 0;            
        arrived = 0;         
        si_clet = 0.0;           
        si_cloud = 0.0;          
        s_setup = 0.0;           
        area_setup = 0.0;
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
            
            for (i = 0; i < 4; i++)                         /* update integral  */
                area[i] += (t.next - t.current) * n[i];
            area_setup += (t.next - t.current) * n_setup;

            t.current = t.next;                             /* advance the clock */


            switch (e->type) {

            case E_ARRIVL:                                  /* process an arrival */

                e->job.id = arrived;
                arrived++;

                if (e->job.class == J_CLASS1) {            /* process a class 1 arrival */
                    //fprintf(stderr, "class 1 arrival\n");
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
                                n_setup++;
                                n_int++;
                            }
                            else { // accept job
                                srvjob(e->job, CLET, &queue, t);
                                a[J_CLASS1 + CLET]++;
                                n[J_CLASS1 + CLET]++;
                            }
                }
                else {                 /* process a class 2 arrival */
                    //fprintf(stderr, "class 2 arrival\n");
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
                
                //if (e->time <= STOP) 
                if (arrived >= N_JOBS) {
                    e->type = E_IGNRVL; // arrival to not process
                    t_stop = t.current; 
                }
                enqueue_event(e, &queue);

                break;

           case E_IGNRVL:
                if (n[J_CLASS1 + CLET] + n[J_CLASS2 + CLET] + 
                    n[J_CLASS1 + CLOUD] + n[J_CLASS2 + CLOUD] + 
                    n_setup) {      // there are jobs to be processed
                    e->time = GetArrival(&jobclass); /* set next arrival to ignore */
                    enqueue_event(e, &queue);
                }
                else free(e);
                break;

           case E_SETUP:
                si_cloud += srvjob(e->job, CLOUD, &queue, t);
                n[J_CLASS2 + CLOUD]++;
                n_setup--;
                break;

            case E_DEPART:                /* process a departure */
                //node = e->node == CLET ? "cloudlet" : "cloud";
                //fprintf(stderr, "class %d departure from %s\n", e->job + 1, node);
                n[e->job.class + e->job.node]--;
                c[e->job.class + e->job.node]++;
                if (t.current <= t_stop)
                    c_stop[e->job.class + e->job.node]++;

                // populate s to print results to stdout
                for (i = 0; i < 4; i++) 
                    s[i] += e->job.service[i];
                s_setup = e->job.setup;

                // write data to outfile
                dprintf(fd_srv, "%ld %f %f %f %f %f\n", e->job.id, 
                    e->job.service[J_CLASS1 + CLET], 
                    e->job.service[J_CLASS2 + CLET], 
                    e->job.service[J_CLASS1 + CLOUD], 
                    e->job.service[J_CLASS2 + CLOUD], e->job.setup);
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
                    area_setup / elapsed);
                dprintf(fd_int, "%f\n", (double) n_int / a[J_CLASS2 + CLET]);
                    
                free(e);
                break;
            
            default:
                handle_error("unknown event type");
            }

        }

        // write completions
        dprintf(fd_srv, "%d %ld %ld %ld %ld %ld\n", -1,
                c[J_CLASS1 + CLET], c[J_CLASS2 + CLET], 
                c[J_CLASS1 + CLOUD], c[J_CLASS2 + CLOUD],
                n_int);

        // close output files
        if (close(fd_srv) == -1)
            handle_error("closing output file");
        if (close(fd_thr) == -1)
            handle_error("closing output file");
        if (close(fd_pop) == -1)
            handle_error("closing output file");
        if (close(fd_int) == -1)
            handle_error("closing output file");
        
        /* sort service output data */
        sprintf(shell_cmd, 
            "sort -n data/srvtemp.dat > data/service_%d.dat; rm data/srvtemp.dat", r);
        if (system(shell_cmd) == -1)
            handle_error("system() - executing sort command");



        /****************** print results *****************/

        double service = 0.0;           /* total service        */
        unsigned long completions = 0;  /* total completions    */
        unsigned long compl_stop = 0;   /* total completions in [START, STOP]  */
        double area_tot = 0.0;          /* time integrated # in the system */

        for (i = 0; i < 4; i++) {
            completions += c[i];
            compl_stop += c_stop[i];
            area_tot += area[i];
            service += s[i];
        }
        area_tot += area_setup;
        service += s_setup;
        

        printf("\nSimulation number %d run with N=%d, S=%d, t_start=%.2f, t_stop=%.2f\n\n", 
                R, N, S, t_start, t_stop);

        printf("\nSystem statistics\n\n");
        printf("  Arrived jobs ....................... = %ld\n", arrived);
        printf("  Processed jobs ..................... = %ld\n", completions);

        printf("  Arrival rate ....................... = %f\n", arrived / (t_stop - t_start));
        printf("  Departure rate ..................... = %f\n\n", compl_stop / (t_stop - t_start));

        printf("  Class 1 system response time ....... = %f\n", 
            (s[J_CLASS1 + CLET] + s[J_CLASS1 + CLOUD]) / 
            (c[J_CLASS1 + CLET] + c[J_CLASS1 + CLOUD]));
        printf("  Class 2 system response time ....... = %f\n", 
            (s[J_CLASS2 + CLET] + s[J_CLASS2 + CLOUD] + s_setup) / 
            (c[J_CLASS2 + CLET] + c[J_CLASS2 + CLOUD]));
        printf("  Global system response time ........ = %f\n", service / arrived);
        printf("  System mean population ............. = %f\n", area_tot / (t.current - t_start));
        printf("  Little's Law: arrival rate = N/S ... = %f (%.2f)\n\n", 
                area_tot * arrived / service / (t.current - t_start), L1 + L2);

        printf("  Class 1 system throughput .......... = %f\n", 
            (c_stop[J_CLASS1 + CLET] + c_stop[J_CLASS1 + CLOUD]) / (t_stop - t_start));
        printf("  Class 2 system throughput .......... = %f\n",  
            (c_stop[J_CLASS2 + CLET] + c_stop[J_CLASS2 + CLOUD]) / (t_stop - t_start));
        printf("  Global system throughput ........... = %f\n\n", 
            compl_stop / (t_stop - t_start));
            
        printf("  Class 1 cloudlet throughput ........ = %f\n", 
            c_stop[J_CLASS1 + CLET] / (t_stop - t_start));
        printf("  Class 2 cloudlet throughput ........ = %f\n", 
            c_stop[J_CLASS2 + CLET] / (t_stop - t_start));
        printf("  Global cloudlet throughput ......... = %f\n\n", 
            (c_stop[J_CLASS1 + CLET] + c_stop[J_CLASS2 + CLET]) / (t_stop - t_start));
        
        printf("  Class 2 arrivals on the cloudlet ... = %ld\n", a[J_CLASS2 + CLET]);
        printf("  Interrupted tasks .................. = %ld\n", n_int);
        printf("  Interrupted tasks percentage ....... = %f\n", (double) 
            n_int / a[J_CLASS2 + CLET]);
        printf("  Interrupted tasks response time .... = %f\n\n", 
            (si_clet + s_setup + si_cloud) / n_int);


        printf("  Class 1 cloudlet response time ..... = %f\n", 
            s[J_CLASS1 + CLET] / c[J_CLASS1 + CLET]);
        printf("  Consistency check: M1CLET = 1/S = .. = %f (%.2f)\n", 
            a[J_CLASS1 + CLET] / s[J_CLASS1 + CLET], M1CLET);

        printf("  Class 1 cloud response time ........ = %f\n", 
            s[J_CLASS1 + CLOUD] / c[J_CLASS1 + CLOUD]);
        printf("  Consistency check: M1CLOUD = 1/S ... = %f (%.2f)\n", 
            c[J_CLASS1 + CLOUD] / s[J_CLASS1 + CLOUD], M1CLOUD);

        printf("  Class 2 cloudlet response time ..... = %f\n", 
            (s[J_CLASS2 + CLET] - si_clet) / c[J_CLASS2 + CLET]);
        printf("  Consistency check: M2CLET = 1/S .... = %f (%.2f)\n", 
            c[J_CLASS2 + CLET] / (s[J_CLASS2 + CLET] - si_clet), M2CLET);

        printf("  Class 2 cloud response time ........ = %f\n", 
            s[J_CLASS2 + CLOUD] / c[J_CLASS2 + CLOUD]);
        printf("  Consistency check: M2CLOUD = 1/S ... = %f (%.2f)\n", 
            c[J_CLASS2 + CLOUD] / s[J_CLASS2 + CLOUD], M2CLOUD);


        printf("  Class 1 cloudlet mean population ... = %f\n", 
            area[J_CLASS1 + CLET] / (t.current - t_start));
        printf("  Class 1 cloud mean population ...... = %f\n", 
            area[J_CLASS1 + CLOUD] / (t.current - t_start));
        printf("  Class 2 cloudlet mean population ... = %f\n", 
            area[J_CLASS2 + CLET] / (t.current - t_start));
        printf("  Class 2 cloud mean population ...... = %f\n\n", 
            area[J_CLASS2 + CLOUD] / (t.current - t_start));

        
        printf("  Class 1 cloudlet arrivals .......... = %ld\n", a[J_CLASS1 + CLET]);
        printf("  Class 2 cloudlet arrivals .......... = %ld\n", a[J_CLASS2 + CLET]); 
        printf("  Class 1 cloud arrivals ............. = %ld\n", a[J_CLASS1 + CLOUD]);
        printf("  Class 2 cloud arrivals ............. = %ld\n\n", a[J_CLASS2 + CLOUD]);

        printf("  Class 1 cloudlet completions ....... = %ld\n", c[J_CLASS1 + CLET]);
        printf("  Class 2 cloudlet completions ....... = %ld\n", c[J_CLASS2 + CLET]); 
        printf("  Class 1 cloud completions .......... = %ld\n", c[J_CLASS1 + CLOUD]);
        printf("  Class 2 cloud completions .......... = %ld\n\n", c[J_CLASS2 + CLOUD]);
    }

    return EXIT_SUCCESS;
}
