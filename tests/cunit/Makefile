CC = gcc
CFLAGS = -I/usr/include/CUnit -Wall
LDFLAGS = -lcunit

all: test_example

test_example: test_example.o
	$(CC) -o test_example test_example.o $(LDFLAGS)

test_example.o: test_example.c
	$(CC) $(CFLAGS) -c test_example.c

clean:
	rm -f *.o test_example