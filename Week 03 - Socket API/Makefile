CC = gcc
CFLAGS = -Wall -g
TARGET = resolver

all: $(TARGET)

$(TARGET): resolver.o
	$(CC) $(CFLAGS) -o $(TARGET) resolver.o

resolver.o: resolver.c
	$(CC) $(CFLAGS) -c resolver.c

clean:
	rm -f *.o $(TARGET)