TARGET = account_manager
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
SRCS = main.c
OBJS = $(SRCS:.c=.o)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)
run: $(TARGET)
	./$(TARGET)
