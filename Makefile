CC = gcc
CFLAGS = -Wall -Iinclude

SRC = src/myHeap.c
TEST = tests/myHeap_test.c
TARGET = test_runner

all: $(TARGET)

$(TARGET): $(SRC) $(TEST)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(TEST)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)