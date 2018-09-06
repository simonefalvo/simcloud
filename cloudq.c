#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rvgs.h"
#include "rngs.h"
#include "basic.h"
#include "utils.h"
#include "queue.h"
#include "eventq.h"




double GetArrival(int *j)
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
	const double mean[4] = {1/M1CLET, 1/M2CLET, 1/M1CLOUD, 1/M2CLOUD};	
    SelectStream(j + n + 2);
    return Exponential(mean[j + n]);
}



double GetSetup()
{
    return Exponential(1/SETUP);
}



void srvjob(int class, int node, struct queue_t *queue, clock t)
{
    double service = GetService(class, node);
    struct event *e = alloc_event();

    e->job = class;
    e->time = t.current + service;
    e->s_start = t.current;
    e->type = E_DEPART;
    e->node = node;

    enqueue_event(e, queue);
}



void rplcjob(struct queue_t *queue, clock t)
{
    double setup = GetSetup();
    struct event *e = alloc_event();
    
    e->job = J_CLASS2;
    e->type = E_DEPART;
    e->node = CLET;
    if (remove_node(e, queue, event_cmp) == -1)
        handle_error("remove_node()");

    e->time = t.current + setup;
    e->type = E_SETUP;
    enqueue_event(e, queue);
}







int main(void)
{
    clock t;

    int jobclass;

    long n1_clet = 0;        /* number of class 1 job in the cloudlet */
    long n2_clet = 0;        /* number of class 2 job in the cloudlet */
    long n1_cloud = 0;        /* number of class 1 job in the cloudlet */
    long n2_cloud = 0;        /* number of class 2 job in the cloudlet */
    long n_int = 0;          /* number interrupted jobs               */

    long arrived;
    long a1_clet = 0;        /* arrived jobs counter                  */
    long a2_clet = 0;        /* arrived jobs counter                  */
    long a1_cloud = 0;       /* arrived jobs counter                  */
    long a2_cloud = 0;       /* arrived jobs counter                  */

    long processed;
    long c1_clet = 0;
    long c2_clet = 0;
    long c1_cloud = 0;
    long c2_cloud = 0;

    double s1_clet = 0.0;
    double s2_clet = 0.0;
    double s1_cloud = 0.0;
    double s2_cloud = 0.0;
    double si_clet = 0.0;
    double si_cloud = 0.0;
    double s_setup = 0.0;

    double area;            /* time integrated number in the node    */
    double area1_clet = 0.0;
    double area2_clet = 0.0;
    double area1_cloud = 0.0;
    double area2_cloud = 0.0;

    double t_last = STOP;    /* last arrival time */

    struct event *e;
    struct queue_t queue;

    /* initialize event queue */
    queue.head = queue.tail = NULL;


    PlantSeeds(0);
    t.current = START;
    e = alloc_event();
    e->time = GetArrival(&jobclass);
    e->type = E_ARRIVL;
    e->job = jobclass;

    enqueue_event(e, &queue);


    while (queue.head != NULL) {

        e = dequeue_event(&queue); 
        t.next = e->time;                               /* next event time */

        area1_clet += (t.next - t.current) * n1_clet;   /* update integral  */
        area2_clet += (t.next - t.current) * n2_clet;   /* update integral  */
        area1_cloud += (t.next - t.current) * n1_cloud; /* update integral  */
        area2_cloud += (t.next - t.current) * n2_cloud; /* update integral  */

        t.current = t.next;                             /* advance the clock */


        switch (e->type) {

        case E_ARRIVL:            /* process an arrival */

            if (e->job == J_CLASS1) { /* process a class 1 arrival */
                fprintf(stderr, "class 1 arrival\n");
                if (n1_clet == N) {
                    srvjob(J_CLASS1, CLOUD, &queue, t);
                    a1_cloud++;
                    n1_cloud++;
                }
                else if (n1_clet + n2_clet < S) { // accept job
                        srvjob(J_CLASS1, CLET, &queue, t);
                        a1_clet++;
                        n1_clet++;
                    }
                    else if (n2_clet > 0) { // replace a class 2 job 
                            rplcjob(&queue, t);
                            srvjob(J_CLASS1, CLET, &queue, t);
                            a1_clet++;
                            n1_clet++;
                            n2_clet--;
                            n_int++;
                        }
                        else { // accept job
                            srvjob(J_CLASS1, CLET, &queue, t);
                            a1_clet++;
                            n1_clet++;
                        }
            }
            else {                 /* process a class 2 arrival */
                fprintf(stderr, "class 2 arrival\n");
                if (n1_clet + n2_clet >= S) {
                    srvjob(J_CLASS2, CLOUD, &queue, t);
                    a2_cloud++;
                    n2_cloud++;
                }
                else {
                    srvjob(J_CLASS2, CLET, &queue, t);
                    a2_clet++;
                    n2_clet++;
                }
            }
            e->time = GetArrival(&jobclass); /* set next arrival t */
            e->job = jobclass;               /* set next arrival j */
            if (e->time <= STOP) {
                enqueue_event(e, &queue);
                t_last = e->time;
            }
            fprint_queue(stderr, &queue, fprint_clet);
            fprintf(stderr, "cloudlet jobs: n1 = %ld, n2 = %ld\n\n", n1_clet, n2_clet);
            fprint_queue(stderr, &queue, fprint_cloud);
            fprintf(stderr, "cloud jobs: n1 = %ld, n2 = %ld\n\n", n1_cloud, n2_cloud);
            break;

       case E_SETUP:
            fprintf(stderr, "setup\n");
            srvjob(J_CLASS2, CLOUD, &queue, t);
            n2_cloud++;
            fprint_queue(stderr, &queue, fprint_clet);
            fprintf(stderr, "cloudlet jobs: n1 = %ld, n2 = %ld\n\n", n1_clet, n2_clet);
            fprint_queue(stderr, &queue, fprint_cloud);
            fprintf(stderr, "cloud jobs: n1 = %ld, n2 = %ld\n\n", n1_cloud, n2_cloud);
            break;

        case E_DEPART:                /* process a departure */
            if (e->node == CLET) {
                if (e->job == J_CLASS1) {
                    fprintf(stderr, "class 1 departure from cloudlet\n");
                    n1_clet--;
                    c1_clet++;
                }
                else {
                    fprintf(stderr, "class 2 departure from cloudlet\n");
                    n2_clet--;
                    c2_clet++;
                }
            }
            else {
                if (e->job == J_CLASS1) {
                    fprintf(stderr, "class 1 departure from cloud\n");
                    n1_cloud--;
                    c1_cloud++;
                }
                else {
                    fprintf(stderr, "class 2 departure from cloud\n");
                    n2_cloud--;
                    c2_cloud++;
                }
            }
            free(e);
            fprint_queue(stderr, &queue, fprint_clet);
            fprintf(stderr, "cloudlet jobs: n1 = %ld, n2 = %ld\n\n", n1_clet, n2_clet);
            fprint_queue(stderr, &queue, fprint_cloud);
            fprintf(stderr, "cloud jobs: n1 = %ld, n2 = %ld\n\n", n1_cloud, n2_cloud);
            break;
        
        default:
            handle_error("unknown event type");
        }
    }

    arrived = a1_clet + a2_clet + a1_cloud + a2_cloud;
    processed = c1_clet + c2_clet + c1_cloud + c2_cloud;
    area = area1_clet + area2_clet + area1_cloud + area2_cloud;

    printf("\nArived jobs statistics are:\n\n");
    printf("  Arrived jobs ...... = %ld\n", arrived);
    printf("  Processed jobs .... = %ld\n", processed);

    printf("\nservice node statistics are:\n\n");
    printf("  avg interarrivals .. = %6.2f\n", t_last / arrived);
    printf("  avg wait ........... = %6.2f\n", area / processed); // approx
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    return EXIT_SUCCESS;
}
