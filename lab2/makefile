CC = gcc
CFLAGS = -std=c11 -pedantic -Wall -Wextra -Werror -Wno-unused-parameter

.PHONY: run
run:
	make child
	make parent
child: 
	$(CC) $(CFLAGS) child.c -o mychild

parent:
	$(CC) $(CFLAGS) parent.c -o myparent 
 
