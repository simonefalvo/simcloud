#ifndef _BASIC_H
#define _BASIC_H

#include <stdlib.h>

#define START       0.0         /* initial (open the door) time     */
#define STOP        2000000.0   /* terminal (close the door) time   */
#define N_JOBS      1000000     /* number of jobs to be processed   */
#define N           20          /* number of servers                */
#define S           20          /* threshold                        */
#define R           10          /* number of repolications          */
#define K           64          /* sample dimension                 */
#define ALPHA       0.05        /* significance level               */
#define L1          4.0         /* class 1 arrival rate             */
#define L2          6.25        /* class 2 arrival rate             */
#define M1CLET      0.45        /* class 1 cloudlet service rate    */
#define M2CLET      0.27        /* class 2 cloudlet service rate    */
#define M1CLOUD     0.25        /* class 1 cloud service rate       */
#define M2CLOUD     0.22        /* class 2 cloud service rate       */
#define SETUP       0.8         /* class 2 mean setup time          */
#define J_CLASS1    0           /* job class type 1                 */
#define J_CLASS2    1           /* job class type 2                 */ 
#define CLET        0           /* cloudlet index                   */
#define CLOUD       2           /* cloud index                      */
#define SRV_IDLE    0           /* idle server index                */
#define SRV_BUSY    1           /* busy server index                */
#define E_ARRIVL    0           /* arrival event type               */
#define E_DEPART    1           /* departure event type             */
#define E_SETUP     2           /* setup event type                 */
#define E_IGNRVL    3           /* arrival to ignore event type     */
#define EDF_PLCY    0           /* Earlest Deadline First preemption policy */
#define LDF_PLCY    0           /* Latest Deadline First preemption policy  */

#define OUTPUT  0
#define DISPLAY 1
#define AUDIT   0

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


typedef struct {                /* simulation clock                 */
    double current;             /*   current time                   */
    double next;                /*   next upcoming event time       */
} clock;

struct job_t {
    unsigned long id;
    unsigned int class;
    unsigned int node;
    double service[4];
    double setup;
};

struct event {                  /* event            */
    double time;                /*   happening time */
    struct job_t job;           /*   related job    */
    unsigned int type;          /*   event type     */
};


#endif /* _BASIC_H */
