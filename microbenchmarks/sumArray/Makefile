CC = clang 

CFLAG = -g -O3 -Wall -Wextra -Werror

all: sumArray

sumArray: functions.o main.o
	$(CC) $(CFLAGS) -o sumArray functions.o main.o

main.o: main.c functions.h 
	$(CC) $(CFLAGS) -c main.c

functions.o: functions.c
	$(CC) $(CFLAGS) -c functions.c

clean: 
	$(RM) sumArray *.o *~

run: 
	./sumArray
