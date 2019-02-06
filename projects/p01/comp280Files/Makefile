CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c11
TARGETS = ttsh siesta

TTSH_SRC = ttsh.c history_queue.c parse_args.c

all: $(TARGETS)

ttsh: $(TTSH_SRC) history_queue.h
	$(CC) $(CFLAGS) -o $@ $(TTSH_SRC)

siesta: siesta.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGETS)
