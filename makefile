#TODO
CC = gcc

#Wvla for checking using VLA. fsanitize=address used to check buffer overflow.
CFLAGS = -DDEBUG -pedantic -Wvla -fsanitize=address

TARGET = sound_seg

SRC = sound_seg.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

#make sure clean is always executed
.PHYON: clean
