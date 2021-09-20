# For C programs 
CC = clang

# flags
CFLAGS = -g -Wall -Wextra -Werror

all: staticArray

staticArray: main.o array.o
	$(CC) $(CFLAGS) -o staticArray array.o main.o

main.o: main.c array.h
	$(CC) $(CFLAGS) -c main.c 

array.o: array.c
	$(CC) $(CFLAGS) -c array.c

clean: 
	$(RM) staticArray *.o *~ 

run:
	./staticArray
