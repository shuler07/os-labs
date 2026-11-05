CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -pthread
LDLIBS = -pthread

all: parent.exe child.exe

parent.exe: src/parent.c src/shared.c src/shared.h
	$(CC) $(CFLAGS) src/parent.c src/shared.c -o parent $(LDLIBS)

child.exe: src/child.c src/shared.c src/shared.h
	$(CC) $(CFLAGS) src/child.c src/shared.c -o child $(LDLIBS)

clean:
	rm -f parent child

.PHONY: all clean
