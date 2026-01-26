CC = mpicc
CFLAGS = -g -Wall -std=c99
LDFLAGS = -lm

SRCS = count_min_sketch.c main.c
# SRCS = count_min_sketch.c mainV2.c

TARGET = main # Executable name

# Build rules
.PHONY: all $(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)