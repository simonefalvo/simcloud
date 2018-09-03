#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rvgs.h"
#include "rngs.h"
#include "basic.h"
#include "utils.h"




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



int NextEvent(event_list event)
/* ---------------------------------------
 * return the index of the next event type
 * ---------------------------------------
 */
{
    int e;
    int i = 0;

    while (event[i].x == 0)     /* find the index of the first 'active' */
        i++;                    /* element in the event list            */
    e = i;
    while (i < N) {             /* now, check the others to find which  */
        i++;                    /* event type is most imminent          */
        if ((event[i].x == 1) && (event[i].t < event[e].t))
            e = i;
    }
    return e;
}



int FindOne(event_list event)
/* -----------------------------------------------------
 * return the index of the available server idle longest
 * -----------------------------------------------------
 */
{
    int s;
    int i = 1;

    while (event[i].x == 1)     /* find the index of the first available */
        i++;                    /* (idle) server                         */
    s = i;
    while (i < N) {             /* now, check the others to find which   */
        i++;                    /* has been idle longest                 */
        if ((event[i].x == 0) && (event[i].t < event[s].t))
            s = i;
    }
    return s;
}



int find_idle(event_list event)
/* ---------------------------------------------------
 * return the index of the first available idle server 
 * ---------------------------------------------------
 */
{
    int s = 1;
    while (event[s].x == SRV_BUSY)     
        s++;                          
    return s;
}



int find_busy(event_list event, int jobclass)
/* -----------------------------------------
 * return the index of the first busy server 
 * with a job of the specified class
 * -----------------------------------------
 */
{
    int s = 1;
    while (event[s].x == SRV_IDLE || event[s].j != jobclass)
        s++;
    return s;
}



void srvjob(event_list event, srvstat_list sum, clock t)
{
    double service = GetService(event[0].j, CLET);
    int s = find_idle(event);
    sum[s].service += service;
    sum[s].served++;
    event[s].t = t.current + service;
    event[s].x = SRV_BUSY;
    event[s].j = event[0].j; 
}



void rplcjob(event_list event, srvstat_list sum, clock t)
{
    double service = GetService(event[0].j, CLET);
    int s = find_busy(event, J_CLASS2);
    sum[s].service -= event[s].t - t.current;
    sum[s].service += service;
    event[s].t = t.current + service;
    event[s].j = event[0].j; 
}







int main(void)
{
    clock t;
    event_list event;
    srvstat_list sum;

    int jobclass;
    long n1 = 0;             /* number of class 1 job in the cloudlet */
    long n2 = 0;             /* number of class 2 job in the cloudlet */
    long ncloud = 0;         /* number of jobs into the cloud         */
    int e;                   /* next event index                      */
    int s;                   /* server index                          */
    long arrived = 0;        /* arrived jobs counter                  */
    long processed = 0;      /* processed jobs counter                */
    long lost = 0;           /* lost jobs counter                     */
    double area = 0.0;       /* time integrated number in the node    */
    double t_last = STOP;    /* last arrival time                     */

    PlantSeeds(0);
    t.current = START;
    event[0].t = GetArrival(&jobclass);
    event[0].x = SRV_BUSY;
    event[0].j = jobclass;

    for (s = 1; s <= N; s++) {
        event[s].t = START;         /* this value is arbitrary because */
        event[s].x = SRV_IDLE;      /* all servers are initially idle  */
        sum[s].service = 0.0;
        sum[s].served = 0;
    }

    while ((event[0].x != 0) || (n1 + n2 + ncloud != 0)) {
        e = NextEvent(event);                     /* next event index */
        t.next = event[e].t;                      /* next event time  */
        area += (t.next - t.current) * (n1 + n2); /* update integral  */
        t.current = t.next;                       /* advance the clock */


        if (e == 0) {           /* process an arrival */
            arrived++;

            if (event[0].j == J_CLASS1) { /* process a class 1 arrival */
                fprintf(stderr, "class 1 arrival\n");
                if (n1 == N) 
                    lost++;
                else if (n1 + n2 < S) { // accept job
                        srvjob(event, sum, t);
                        n1++;
                    }
                    else if (n2 > 0) { // replace a class 2 job 
                            rplcjob(event, sum, t);
                            n1++;
                            n2--;
                            lost++;
                        }
                        else { // accept job
                            srvjob(event, sum, t);
                            n1++;
                        }
            }
            else {                 /* process a class 2 arrival */
                fprintf(stderr, "class 2 arrival\n");
                if (n1 + n2 >= S)
                    lost++;
                else {
                    srvjob(event, sum, t);
                    n2++;
                }
            }

            event[0].t = GetArrival(&jobclass);  /* set next arrival t */
            event[0].j = jobclass;               /* set next arrival j */

            if (event[0].t > STOP)  
                event[0].x = 0; /* turn off the event */
            else t_last = event[0].t;

            fprint_servers(stderr, event);
            fprintf(stderr, "pending jobs: %ld\n\n", n1 + n2);

        } else {                /* process a departure */
            fprintf(stderr, "departure\n");
            s = e;
            event[s].x = SRV_IDLE;
            if (event[s].j == J_CLASS1)
                n1--;
            else 
                n2--;
            processed++;            

            fprint_servers(stderr, event);
            fprintf(stderr, "pending jobs: %ld\n\n", n1 + n2);
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

    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= N; s++)
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double) sum[s].served / processed);
    printf("\n");

    return EXIT_SUCCESS;
}
