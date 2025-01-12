

CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = eshop

SRCS = eshop.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

#run make -f eshop.make on terminal
#./eshop
