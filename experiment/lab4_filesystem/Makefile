CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2

TARGET = filesystem
SRC = filesystem.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET) < sample_input.txt

clean:
	rm -f $(TARGET)
