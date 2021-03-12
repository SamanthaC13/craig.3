CC	= gcc
CFLAGS	= -g
TARGET	= all
OBJS	= monitor producer consumer

$(TARGET) : $(OBJS)

libmonitor : libmonitor.c
	$(CC) -o libmonitor.o -c libmonitor.c
	ar rcs libmonitor.a libmonitor.o

monitor : main.c
	$(CC) -o monitor main.c -L. -lmonitor -lrt -lpthread

producer : producer.c
	$(CC) -o producer producer.c -L. -lmonitor -lrt -lpthread

consumer : consumer.c
	$(CC) -o consumer consumer.c -L. -lmonitor -lrt -lpthread -lm
clean :
	/bin/rm -f *.o *.log *.a $(OBJS)
