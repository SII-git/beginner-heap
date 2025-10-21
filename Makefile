CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -fPIC -Iinclude
LDFLAGS =

SRC = src/myheap.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean test stress asan asan-stress

all: libmyheap.so test

libmyheap.so: $(OBJ)
	$(CC) -shared -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c include/myheap.h
	$(CC) $(CFLAGS) -c $< -o $@

test: libmyheap.so tests/test_myheap.c
	$(CC) -g -Iinclude -L. -o test_myheap tests/test_myheap.c -lmyheap -Wl,-rpath,.

stress: libmyheap.so tests/stress.c
	$(CC) -g -Iinclude -L. -o stress tests/stress.c -lmyheap -Wl,-rpath,.

asan:
	$(CC) $(CFLAGS) -fsanitize=address -o test_asan tests/test_myheap.c src/myheap.c

asan-stress:
	$(CC) $(CFLAGS) -fsanitize=address -o stress_asan tests/stress.c src/myheap.c

clean:
	rm -f src/*.o libmyheap.so test_myheap stress test_asan stress_asan
