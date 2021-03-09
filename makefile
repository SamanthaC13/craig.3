CC	= gcc
CFLAGS	= -g
TARGET	= all
OBJS	= monitor producer consumer

$(TARGET) : $(OBJS)

libmonitor : libmonitor.c
	$(CC) -o monitor_lib -c monitor_lib.c
	ar rcs monitor_lib.a monitor_lib.o

monitor : main.c
	$(CC) -o monitor main.c -L. -lmonitor -lrt -lpthread

producer : producer.c
	$(CC) -o producer producer.c -lrt -lpthread

consumer : consumer.c
	$(CC) -o consumer consumer.c -lrt -lpthread -lm

clean :
	/bin/rm -f *.o *.log *.a $(OBJS)
