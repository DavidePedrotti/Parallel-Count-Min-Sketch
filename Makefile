CC = mpicc
CFLAGS = -g -Wall -std=c99
LDFLAGS = -lm

SRCS = count_min_sketch.c mainV3.c
# SRCS = count_min_sketch.c mainV2.c

TARGET = mainV3 # Executable name

# Build rules
.PHONY: all $(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)