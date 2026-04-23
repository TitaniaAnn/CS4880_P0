CC     = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g

.PHONY: all clean test

all: P0

P0: P0.o tree.o
	$(CC) $(CFLAGS) -o P0 P0.o tree.o

P0.o: P0.c tree.h node.h
	$(CC) $(CFLAGS) -c P0.c

tree.o: tree.c tree.h node.h
	$(CC) $(CFLAGS) -c tree.c

test: P0
	bash run_tests.sh

clean:
	rm -f *.o P0 P0.exe out.preorder out.inorder out.postorder _test.* _stderr.tmp
