#include "queue.h"
#include "basic.h"



void fprint_event(FILE * stream, void *p)
{
    struct event *e = p;
    char type = e->type == E_ARRIVL ? 'a' : 'd';
    char *node = e->node == CLET? "clet" : "cloud"; 

    fprintf(stream, "|%f-", e->time);
    fprintf(stream, "%s-", node);
    fprintf(stream, "%c-", type);
    fprintf(stream, "%d|", e->job + 1);
}

void fprint_clet(FILE * stream, void *p)
{
    struct event *e = p;
    if (e->type == E_DEPART && e->node == CLET)
        fprintf(stream, "-%d-", e->job + 1);
}

void fprint_cloud(FILE * stream, void *p)
{
    struct event *e = p;
    if (e->type == E_DEPART && e->node == CLOUD)
        fprintf(stream, "-%d-", e->job + 1);
}


struct event *alloc_event()
{
    struct event *e = malloc(sizeof(struct event));
    if (!e)
        handle_error("allocating an event struct");
    return e;
}


/*
 * Function:	time_cmp	
 * -------------------------------------
 * Compare the trigger time of two events.
 *
 * Parameters:
 * 		xp	the address of the first packet
 * 		yp	the address of the second packet
 *
 * 	Returns:
 * 		1	xp exptime > yp exptime
 * 		0	xp exptime = yp exptime
 * 		-1	xp exptime < yp exptime
 */
int time_cmp(void *xp, void *yp)
{
    struct event *x = xp;
    struct event *y = yp;

    if (x->time > y->time)
        return 1;
    else if (x->time < y->time)
        return -1;
    else
        return 0;
}


/*
 * Function:	event_cmp	
 * -------------------------------------
 * Compare two events.
 * Two events are equals iff type, node and jobclass are.
 *
 * Parameters:
 * 		xp	the address of the first packet
 * 		yp	the address of the second packet
 *
 * 	Returns:
 * 		0	xp->job == yp->job && xp->type == yp->type &&
 *          xp->node == yp->node
 * 	    !0	otherwise
 */
int event_cmp(void *xp, void *yp)
{
    struct event *x = xp;
    struct event *y = yp;

    return (x->job != y->job)
        || (x->type != y->type) 
        || (x->node != y->node);
}


struct event *remove_event(struct queue_t *queue, struct event *e, unsigned int n)
{
    struct node_t *removed = NULL;

    if (remove_noden(e, queue, &removed, n, event_cmp) == -1)
        handle_error("removing an event from the queue");

    if (!removed)
        handle_error("returned null event");

    return removed->value;
}


/*
 * Function:	dequeue_event
 * ----------------------------------------------
 * Dequeue the first element of the queue and 
 * return the related event address.
 *
 * Parameters:
 * 		queue	the address of the queue
 *
 * Returns:
 * 		the address of the related event,
 * 		ie the node's value.
 */
struct event *dequeue_event(struct queue_t *queue)
{
    struct event *e;
    struct node_t *node = NULL;

    if (dequeue_node(queue, &node) == -1)
        handle_error("dequeue_node()");

    e = node->value;
    free(node);

    return e;
}



void enqueue_event(struct event *e, struct queue_t *queue)
{
    if (prio_enqueue(e, queue, time_cmp) == -1)
        handle_error("prio_enqueue()");
}
