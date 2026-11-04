CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L -pthread
LDLIBS = -pthread

all: parent.exe child.exe

parent.exe: parent.c shared.c shared.h
	$(CC) $(CFLAGS) parent.c shared.c -o parent.exe $(LDLIBS)

child.exe: child.c shared.c shared.h
	$(CC) $(CFLAGS) child.c shared.c -o child.exe $(LDLIBS)

clean:
	rm -f *.exe

.PHONY: all clean
