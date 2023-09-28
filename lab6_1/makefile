CC = gcc

SRC = sortIndex.c

sortIndex: $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run
run:
	make main
	./main
	make clean

clean:
	rm sortIndex.o