#include "utils.h"

void fprint_servers(FILE* stream, event_list event) 
{
    int s;
    for (s = 1; s <= N; s++) {
        if (event[s].x == SRV_IDLE) 
            fprintf(stream, "- -");
        else
            fprintf(stream, "-%d-", event[s].j + 1);
    }
    fprintf(stream, "\n");
}
