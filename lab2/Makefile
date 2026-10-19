CC = gcc
CFLAGS = -std=c11 -O2 -D_POSIX_C_SOURCE=200809L

all: parent child

parent: parent.c
	$(CC) $(CFLAGS) parent.c -o parent

child: child.c
	$(CC) $(CFLAGS) child.c -o child

clean:
	rm -f parent child
