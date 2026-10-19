CC = gcc
CFLAGS = -std=c11 -O2 -D_POSIX_C_SOURCE=200809L

all: parent child

parent: src/parent.c
	$(CC) $(CFLAGS) src/parent.c -o parent

child: src/child.c
	$(CC) $(CFLAGS) src/child.c -o child

clean:
	rm -f parent child
