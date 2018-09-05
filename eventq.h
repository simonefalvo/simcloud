#ifndef _EVENTQ_H
#define _EVENTQ_H

#include <stdio.h>
#include "basic.h"
#include "queue.h"

void fprint_event(FILE * stream, void *p);
void fprint_server(FILE * stream, void *p);
int time_cmp(void *xp, void *yp);
int job_cmp(void *xp, void *yp);
struct event *dequeue_event(struct queue_t *queue);
struct event *get_head_event(struct queue_t *queue);
void enqueue_event(struct event *e, struct queue_t *queue);
struct event *alloc_event();

#endif /* _EVENTQ_H */
