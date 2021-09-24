# For C programs 
CC = clang

# flags
CFLAGS = -g -Wall -Wextra -Werror

all: sumArray

sumArray: main.o mergeSort.o
	$(CC) $(CFLAGS) -o sumArray mergeSort.o main.o

main.o: main.c mergeSort.h
	$(CC) $(CFLAGS) -c main.c 

mergeSort.o: mergeSort.c
	$(CC) $(CFLAGS) -c mergeSort.c

clean: 
	$(RM) sumArray *.o *~ 

run:
	./sumArray
