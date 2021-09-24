# For C programs 
CC = clang

# flags
CFLAGS = -g -O3 -Wall -Wextra -Werror

all: sortingArray

sortingArray: main.o mergeSort.o
	$(CC) $(CFLAGS) -o sortingArray mergeSort.o main.o

main.o: main.c mergeSort.h
	$(CC) $(CFLAGS) -c main.c 

mergeSort.o: mergeSort.c
	$(CC) $(CFLAGS) -c mergeSort.c

clean: 
	$(RM) sortingArray *.o *~ 

run:
	./sortingArray
