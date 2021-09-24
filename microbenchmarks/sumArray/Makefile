CC = clang 

CFLAG = -g -O3 -Wall -Wextra -Werror

all: sumArray

sumArray: main.o
	$(CC) $(CFLAGS) -o sumArray main.o

main.o: main.c 
	$(CC) $(CFLAGS) -c main.c 

clean: 
	$(RM) sumArray *.o *~

run: 
	./sumArray
