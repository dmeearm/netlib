CFLAGS = -g -Wall -Wpointer-arith -Wno-unused-parameter \
    -Wunused-function -Wunused-variable -Wunused-value -Werror

CC = gcc

OBJS = main.o

TARGET = main

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) $< -o $@

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY:clean


