CC = mpicc
CFLAGS = -g -Wall -std=c99
LDFLAGS = -lm

# OpenMP configuration
# CC = gcc
# CFLAGS += -fopenmp
# no LDLFLAGS needed

SRCS = count_min_sketch.c mainV2.c
# SRCS = count_min_sketch.c mainV2.c

TARGET = mainV2 # Executable name

# Build rules
.PHONY: all $(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)