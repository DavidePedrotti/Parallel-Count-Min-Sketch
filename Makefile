CC = mpicc
CFLAGS = -g -Wall -std=c99 -lm

SRCS = count_min_sketch.c main.c
TARGET = cms_mpi # Executable name

# Build rules
.PHONY: all $(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^