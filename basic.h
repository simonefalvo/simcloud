#ifndef _BASIC_H
#define _BASIC_H


#define START    0.0            /* initial (open the door) time   */
#define STOP     10.0           /* terminal (close the door) time */
#define N        20             /* number of servers              */
#define S        5             /* threshold                      */
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
#define CLOUD    1              /* cloud index                    */
#define SRV_IDLE 0              /* idle server index              */
#define SRV_BUSY 1              /* busy server index              */


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


#endif /* _BASIC_H */
