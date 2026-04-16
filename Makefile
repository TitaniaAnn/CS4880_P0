CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g

.PHONY: all clean

all: P0

P0: P0.o tree.o
	$(CC) $(CFLAGS) -o P0 P0.o tree.o

P0.o: P0.c tree.h node.h
	$(CC) $(CFLAGS) -c P0.c

tree.o: tree.c tree.h node.h
	$(CC) $(CFLAGS) -c tree.c

clean:
	rm -f *.o P0 out.preorder out.inorder out.postorder
