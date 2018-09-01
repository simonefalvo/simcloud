CC := gcc
CFLAGS := -Wall -Wextra -O3 -fno-stack-protector

PROGS := msq

all: $(PROGS)

msq: msq.c rngs.c rngs.h -lm

.PHONY: all clean
clean:
	rm -f *~ $(PROGS)
