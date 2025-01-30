

CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = eshop_Erg3

SRCS = eshop_Erg3.c

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
#./eshop_Erg3
