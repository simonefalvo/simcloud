CC := gcc
CFLAGS := -Wall -Wextra -O3 

PROGS := clet \
         cloud \
         cloudq \
         batch_means \
         thr_bm \
         pop_bm

all: $(PROGS)

clet: clet.c rvgs.h rvgs.c rngs.c rngs.h basic.h utils.h utils.c -lm
cloud: cloud.c rvgs.h rvgs.c rngs.c rngs.h basic.h utils.h utils.c queue.c queue.h eventq.h eventq.c -lm
cloudq: cloudq.c rvgs.h rvgs.c rngs.c rngs.h basic.h utils.h utils.c queue.h queue.c eventq.h eventq.c rvms.c rvms.h -lm
batch_means: batch_means.c basic.h rvms.c rvms.h -lm
thr_bm: thr_bm.c basic.h rvms.c rvms.h -lm
pop_bm: pop_bm.c basic.h rvms.c rvms.h -lm

.PHONY: all clean
clean:
	rm -f *~ $(PROGS)
