CC=cc
CFLAGS:=${CFLAGS} -std=c99 -Wall -Wextra
LDFLAGS:=${LDFLAGS} -lm

.PHONY: all clean
.DEFAULT_GOAL: all

all: clean simpletron example translator

simpletron:
	$(CC) $(CFLAGS) $(LDFLAGS) run_simpletron.c simpletron.c -o simpletron

example:
	$(CC) $(CFLAGS) $(LDFLAGS) write_test_programs.c simpletron.c -o mktestprog

translator:
	$(CC) $(CFLAGS) $(LDFLAGS) smlt.c translator.c simpletron.c evaluate.c -o smlt

clean:
	rm simpletron mktestprog smlt 2> /dev/null || echo Already clean
