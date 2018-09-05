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




void srvjob(int jobtype, struct queue_t *queue, clock t)
{
    double service = GetService(jobtype, CLET);
    struct event *e = alloc_event();

    e->job = jobtype;
    e->time = t.current + service;
    e->type = E_DEPART;

    enqueue_event(e, queue);
}



void rplcjob(struct queue_t *queue, clock t)
{
    double service = GetService(J_CLASS1, CLET);
    struct event *e = alloc_event();
    
    e->job = J_CLASS2;
    if (remove_node(e, queue, job_cmp) == -1)
        handle_error("remove_node()");
    // TODO: mettere il job di classe 2 in servizio sul cloud

    e->job = J_CLASS1;
    e->time = t.current + service;
    e->type = E_DEPART;

    enqueue_event(e, queue);
}







int main(void)
{
    clock t;

    int jobclass;
    long n1 = 0;             /* number of class 1 job in the cloudlet */
    long n2 = 0;             /* number of class 2 job in the cloudlet */
    long ncloud = 0;         /* number of jobs into the cloud         */
    long arrived = 0;        /* arrived jobs counter                  */
    long processed = 0;      /* processed jobs counter                */
    long lost = 0;           /* lost jobs counter                     */
    double area = 0.0;       /* time integrated number in the node    */
    double t_last = STOP;    /* last arrival time                     */

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
        t.next = e->time;                         /* next event time  */
        area += (t.next - t.current) * (n1 + n2); /* update integral  */
        t.current = t.next;                       /* advance the clock */


        if (e->type == E_ARRIVL) {           /* process an arrival */
            arrived++;

            if (e->job == J_CLASS1) { /* process a class 1 arrival */
                fprintf(stderr, "class 1 arrival\n");
                if (n1 == N) 
                    lost++;
                else if (n1 + n2 < S) { // accept job
                        srvjob(J_CLASS1, &queue, t);
                        n1++;
                    }
                    else if (n2 > 0) { // replace a class 2 job 
                            rplcjob(&queue, t);
                            n1++;
                            n2--;
                            lost++;
                        }
                        else { // accept job
                            srvjob(J_CLASS1, &queue, t);
                            n1++;
                        }
            }
            else {                 /* process a class 2 arrival */
                fprintf(stderr, "class 2 arrival\n");
                if (n1 + n2 >= S)
                    lost++;
                else {
                    srvjob(J_CLASS2, &queue, t);
                    n2++;
                }
            }

            e->time = GetArrival(&jobclass); /* set next arrival t */
            e->job = jobclass;               /* set next arrival j */

            if (e->time <= STOP) {
                enqueue_event(e, &queue);
                t_last = e->time;
            }

            //fprint_servers(stderr, &queue);
            fprint_queue(stderr, &queue, fprint_server);
            fprintf(stderr, "pending jobs: n1 = %ld, n2 = %ld\n\n", n1, n2);

        } else {                /* process a departure */
            fprintf(stderr, "departure\n");
            if (e->job == J_CLASS1)
                n1--;
            else 
                n2--;
            processed++;            
            free(e);

            fprint_queue(stderr, &queue, fprint_server);
            fprintf(stderr, "pending jobs: n1 = %ld, n2 = %ld\n\n", n1, n2);
        }
    }

    printf("\nArived jobs statistics are:\n\n");
    printf("  Arrived jobs ...... = %ld\n", arrived);
    printf("  Processed jobs .... = %ld\n", processed);
    printf("  Lost jobs ......... = %ld\n", lost);
    printf("  Lost + processed .. = %ld\n", lost + processed);
    printf("  %% Lost ....... = %6.2f\n", (double) lost / arrived);
    printf("  %% Processed .. = %6.2f\n", (double) processed / arrived);

    printf("\nservice node statistics are:\n\n");
    printf("  avg interarrivals .. = %6.2f\n", t_last / arrived);
    printf("  avg wait ........... = %6.2f\n", area / processed); // approx
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    return EXIT_SUCCESS;
}
