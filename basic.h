#ifndef _BASIC_H
#define _BASIC_H

#include <stdlib.h>

#define START    0.0            /* initial (open the door) time   */
#define STOP     2000000.0      /* terminal (close the door) time */
#define N_JOBS   500000         /* number of jobs to be processed */
#define N        20             /* number of servers              */
#define S        20             /* threshold                      */
#define R        4              /* number of repolications        */
#define L1       4.0            /* class 1 arrival rate           */
#define L2       6.25           /* class 2 arrival rate           */
#define M1CLET   0.45           /* class 1 cloudlet service rate  */
#define M2CLET   0.27           /* class 2 cloudlet service rate  */
#define M1CLOUD  0.25           /* class 1 cloud service rate     */
#define M2CLOUD  0.22           /* class 2 cloud service rate     */
#define SETUP    0.8            /* class 2 mean setup time        */
#define J_CLASS1 0              /* job class type 1               */
#define J_CLASS2 1              /* job class type 2               */ 
#define CLET     0              /* cloudlet index                 */
#define CLOUD    2              /* cloud index                    */
#define SRV_IDLE 0              /* idle server index              */
#define SRV_BUSY 1              /* busy server index              */
#define E_ARRIVL 0
#define E_DEPART 1
#define E_SETUP  2
#define E_IGNRVL 3


#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


typedef struct {                /* simulation clock                  */
    double current;             /*   current time                    */
    double next;                /*   next (most imminent) event time */
} clock;

typedef struct {                /* the next-event list    */
    double t;                   /*   next event time      */
    int x;                      /*   event status, 0 or 1 */
    int j;                      /*   job class type       */
} event_list[N + 1];

typedef struct {                /* accumulated sums of    */
    double service;             /*   service times        */
    long served;                /*   number served        */
} srvstat_list[N + 1];

struct job_t {
    unsigned long id;
    unsigned int class;
    unsigned int node;
    double service[4];
    double setup;
};

struct event {
    double time;
    struct job_t job;
    unsigned int type;
    unsigned int n[4];
};


#endif /* _BASIC_H */
