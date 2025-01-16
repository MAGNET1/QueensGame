# Variables
CC = gcc
CFLAGS = -Wall -g -Iinc
TARGET = my_program
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=.build/%.o)
RUN_ARGS = 5

# Default Target (build and run)
default: build run

# Build target
build: $(TARGET)

$(TARGET): $(OBJ) | .build
	$(CC) $(CFLAGS) -o $@ $^

.build/%.o: src/%.c | .build
	$(CC) $(CFLAGS) -c $< -o $@

.build:
	mkdir -p .build

# Run target
run: build
	./$(TARGET) $(RUN_ARGS)

# Clean target
clean:
	rm -rf .build

.PHONY: default build run clean
