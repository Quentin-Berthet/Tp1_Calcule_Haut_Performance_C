CC=mpicc
CFLAGS=-std=gnu11 -Wall -Wextra -Wconversion -pedantic -MMD
OUTPUT=mmm.out
C_SRC=$(wildcard *.c)
C_OBJ=$(C_SRC:.c=.o)
C_DEPS=$(C_OBJ:%.o=%.d)

all: clean build

build: $(C_OBJ)
	$(CC) $^ -o $(OUTPUT) -lm

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(OUTPUT) *.o *.d

.PHONY: all clean

-include $(C_DEPS)
