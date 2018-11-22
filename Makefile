CC := gcc
CFLAGS := -Wall -Wextra -O3 

PROGS := cloudq \
         bm_srv \
         bm_thr \
         bm_pop

all: $(PROGS)

cloudq: cloudq.c rvgs.h rvgs.c rngs.c rngs.h basic.h queue.h queue.c eventq.h eventq.c rvms.c rvms.h -lm
bm_srv: bm_srv.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm
bm_thr: bm_thr.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm
bm_pop: bm_pop.c basic.h stats_utils.h stats_utils.c rvms.c rvms.h -lm

.PHONY: all clean
clean:
	rm -f *~ $(PROGS)
