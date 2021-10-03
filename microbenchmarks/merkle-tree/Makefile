CC = clang 

CFLAG = -g -O3 -Wall -Wextra -Werror

TARGET:=merkle

all: $(TARGET)

$(TARGET): $(TARGET).o
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET).o: $(TARGET).c 
	$(CC) $(CFLAGS) -o $@ -c $<

clean: 
	$(RM) $(TARGET) *.o *~

run: 
	./$(TARGET)
