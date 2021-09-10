# For C programs 
CC = clang

# flags
CFLAGS = -g -Wall -Wextra -Werror

all: dynamicArray

dynamicArray: main.o array.o
	$(CC) $(CFLAGS) -o dynamicArray array.o main.o

main.o: main.c array.h
	$(CC) $(CFLAGS) -c main.c 

array.o: array.c
	$(CC) $(CFLAGS) -c array.c

clean: 
	$(RM) dynamicArray *.o *~ 

run:
	./dynamicArray
