CC := gcc
CFLAGS := -Wall -Wextra -O3 -fno-stack-protector

PROGS := clet

all: $(PROGS)

clet: clet.c rvgs.h rvgs.c rngs.c rngs.h basic.h utils.h utils.c -lm

.PHONY: all clean
clean:
	rm -f *~ $(PROGS)
