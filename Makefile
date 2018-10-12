CC := gcc
CFLAGS := -Wall -Wextra -O3 

PROGS := cloudq \
         srv_bm \
         thr_bm \
         pop_bm

all: $(PROGS)

cloudq: cloudq.c rvgs.h rvgs.c rngs.c rngs.h basic.h utils.h utils.c queue.h queue.c eventq.h eventq.c rvms.c rvms.h -lm
srv_bm: srv_bm.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm
thr_bm: thr_bm.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm
pop_bm: pop_bm.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm

.PHONY: all clean
clean:
	rm -f *~ $(PROGS)
