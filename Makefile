CFLAGS = -g -Wall -Wpointer-arith -Wno-unused-parameter \
    -Wunused-function -Wunused-variable -Wunused-value -Werror

CC = gcc

OBJS = main.o net.o 

TARGET = main

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) $^ -o $@

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY:clean

clean:
	rm -f $(TARGET) $(OBJS)



