CC = gcc

# Wvla for checking using VLA. fsanitize=address used to check buffer overflow.
CFLAGS = -DDEBUG -pedantic -Wvla -fsanitize=address -Wall

TARGET = sound_seg

SRC = $(wildcard *.c)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

# make sure clean is always executed
.PHONY: clean