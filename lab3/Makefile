CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -D_GNU_SOURCE -pthread
LDLIBS = -lm -pthread

all: main

main: src/main.c src/monte_carlo.c src/monte_carlo.h
	$(CC) $(CFLAGS) src/main.c src/monte_carlo.c -o main $(LDLIBS)

clean:
	rm -f main

.PHONY: all clean
