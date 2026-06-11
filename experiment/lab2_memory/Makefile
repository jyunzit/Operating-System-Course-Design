CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

TARGET = memory
SRC = memory.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

test_fifo: $(TARGET)
	./$(TARGET) < sample_fifo.txt

test_lru: $(TARGET)
	./$(TARGET) < sample_lru.txt

test_partition: $(TARGET)
	./$(TARGET) < sample_partition.txt

clean:
	rm -f $(TARGET)
