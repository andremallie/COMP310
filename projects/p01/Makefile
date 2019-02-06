CC = gcc
CFLAGS = -g -lreadline -Wall #-Werror
TARGETS = tosh siesta

# add to this list if you create new .c source files
TOSH_SRC = tosh.c

all: $(TARGETS)

ttsh: $(TOSH_SRC) 
	$(CC) $(CFLAGS) -o $@ $(TOSH_SRC)

siesta: siesta.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(TARGETS)
