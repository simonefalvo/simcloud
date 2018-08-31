/* ------------------------------------------------------------------------- 
 * This program is a next-event simulation of a multi-server, single-queue 
 * service node.  The service node is assumed to be initially idle, no 
 * arrivals are permitted after the terminal time STOP and the node is then 
 * purged by processing any remaining jobs. 
 * 
 * Name              : msq.c (Multi-Server Queue)
 * Author            : Steve Park & Dave Geyer 
 * Language          : ANSI C 
 * Latest Revision   : 10-19-98 
 * ------------------------------------------------------------------------- 
 */

#include <stdio.h>
#include <math.h>
#include "rngs.h"

#define START    0.0            /* initial (open the door)        */
#define STOP     20000.0        /* terminal (close the door) time */
#define N        4              /* number of servers              */

typedef struct {                /* the next-event list    */
    double t;                   /*   next event time      */
    int x;                      /*   event status, 0 or 1 */
    int job;                    /*   job class type       */
} event_list[N + 1];


double Exponential(double m)
/* ---------------------------------------------------
 * generate an Exponential random variate, use m > 0.0 
 * ---------------------------------------------------
 */
{
    return (-m * log(1.0 - Random()));
}


double Uniform(double a, double b)
/* --------------------------------------------
 * generate a Uniform random variate, use a < b 
 * --------------------------------------------
 */
{
    return (a + (b - a) * Random());
}

/*
double GetArrival(void)
{
    static double arrival = START;

    SelectStream(0);
    arrival += Exponential(2.0);
    return (arrival);
}
*/

double GetArrival(int *j)
{
	const double mean[2] = {4.0,6.0};	//tassi di arrivo l1 e l2
	static double arrival[2] = {START, START};
	static int init = 1;
	double temp;
	
	if (init) {
		SelectStream(0);
		arrival[0] += Exponential(mean[0]);
		SelectStream(0);
		arrival[0] += Exponential(mean[1]);
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

double GetService(void)
/* ---------------------------------------------
 * generate the next service time, with rate 1/6
 * ---------------------------------------------
 */
{
    SelectStream(2);
    return (Uniform(2.0, 10.0));
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
    return (e);
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
    return (s);
}


int main(void)
{
    int j;

    struct {
        double current;         /* current time                       */
        double next;            /* next (most imminent) event time    */
    } t;
    event_list event;
    long number = 0;            /* number in the node                 */
    int e;                      /* next event index                   */
    int s;                      /* server index                       */
    long index = 0;             /* used to count processed jobs       */
    double area = 0.0;          /* time integrated number in the node */
    struct {                    /* accumulated sums of                */
        double service;         /*   service times                    */
        long served;            /*   number served                    */
    } sum[N + 1];

    PlantSeeds(0);
    t.current = START;
    event[0].t = GetArrival(&j);
    event[0].x = 1;
    for (s = 1; s <= N; s++) {
        event[s].t = START;     /* this value is arbitrary because */
        event[s].x = 0;         /* all servers are initially idle  */
        sum[s].service = 0.0;
        sum[s].served = 0;
    }

    while ((event[0].x != 0) || (number != 0)) {
        e = NextEvent(event);   /* next event index */
        t.next = event[e].t;    /* next event time  */
        area += (t.next - t.current) * number;  /* update integral  */
        t.current = t.next;     /* advance the clock */

        if (e == 0) {           /* process an arrival */
            number++;
            event[0].t = GetArrival(&j);  /* get next arrival */
            if (event[0].t > STOP)
                event[0].x = 0; /* turn off the event */
            if (number <= N) {
                double service = GetService();
                s = FindOne(event);
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
                event[s].x = 1;
            }
        } else {                /* process a departure */
            index++;            /* from server s       */
            number--;
            s = e;
            if (number >= N) {
                double service = GetService();
                sum[s].service += service;
                sum[s].served++;
                event[s].t = t.current + service;
            } else
                event[s].x = 0;
        }
    }

    printf("\nfor %ld jobs the service node statistics are:\n\n", index);
    printf("  avg interarrivals .. = %6.2f\n", event[0].t / index);
    printf("  avg wait ........... = %6.2f\n", area / index);
    printf("  avg # in node ...... = %6.2f\n", area / t.current);

    for (s = 1; s <= N; s++)    /* adjust area to calculate */
        area -= sum[s].service; /* averages for the queue   */

    printf("  avg delay .......... = %6.2f\n", area / index);
    printf("  avg # in queue ..... = %6.2f\n", area / t.current);
    printf("\nthe server statistics are:\n\n");
    printf("    server     utilization     avg service        share\n");
    for (s = 1; s <= N; s++)
        printf("%8d %14.3f %15.2f %15.3f\n", s, sum[s].service / t.current,
               sum[s].service / sum[s].served,
               (double) sum[s].served / index);
    printf("\n");

    return (0);
}
