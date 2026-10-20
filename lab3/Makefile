CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -pthread
LDLIBS = -lm -pthread

all: main

main: main.c monte_carlo.c monte_carlo.h
	$(CC) $(CFLAGS) main.c monte_carlo.c -o main $(LDLIBS)

clean:
	rm -f main

.PHONY: all clean
