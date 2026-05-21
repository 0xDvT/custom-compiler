all:
# Makefile
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -Wpedantic
DEPS = include/scanner.h include/token.h include/parser.h include/codegen.h
OBJ = src/main.o src/scanner.o src/token.o src/parser.o	src/codegen.o
TARGET = ./compiler

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f src/*.o $(TARGET)
	rm -f test.s test.o test

test:
	$(TARGET) test.pas
	./test
	echo $?
