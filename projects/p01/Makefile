CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11
TARGETS = tosh siesta

TOSH_SRC = tosh.c history_queue.c parse_args.c

all: $(TARGETS)

tosh: $(TOSH_SRC) history_queue.h
	$(CC) $(CFLAGS) -o $@ $(TOSH_SRC)

siesta: siesta.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGETS)
