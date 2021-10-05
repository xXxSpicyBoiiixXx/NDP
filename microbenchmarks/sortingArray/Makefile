# For C programs 
CC = clang

# flags
CFLAGS = -g -O3 -Wall -Wextra -Werror

all: sortingArray

sortingArray: main.o sortAlgo.o array_functions.o
	$(CC) $(CFLAGS) -o sortingArray array_functions.o sortAlgo.o main.o

main.o: main.c sortAlgo.h
	$(CC) $(CFLAGS) -c main.c 

sortAlgo.o: sortAlgo.c
	$(CC) $(CFLAGS) -c sortAlgo.c

array_functions.o:
	$(CC) $(CFLAGS) -c array_functions.c 

clean: 
	$(RM) sortingArray *.o *~ 

run:
	./sortingArray
