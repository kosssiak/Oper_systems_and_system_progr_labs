CC = gcc

MAIN = main.c
PRODUCER = producer.c
CONSUMER = consumer.c

.PHONY: run
run:
	make consumer
	make producer
	make parent

consumer:
	$(CC) $(CFLAGS) -o consumer $(CONSUMER) 

producer:
	$(CC) $(CFLAGS) -o producer $(PRODUCER) 

parent:
	$(CC) $(CFLAGS) -o main $(MAIN)