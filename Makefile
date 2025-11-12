CC = gcc
CFLAGS = -g -Wall -std=c99 -lm

SRCS = count_min_sketch.c count_min_sketch.h
TARGET = count_min_sketch # Executable name

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^