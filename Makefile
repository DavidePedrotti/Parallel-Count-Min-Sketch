CC = mpicc
CFLAGS = -g -Wall -std=c99
LDFLAGS = -lm

SRCS = count_min_sketch.c cms_linear.c
# SRCS = count_min_sketch.c mainV2.c

TARGET = cms_linear # Executable name

# Build rules
.PHONY: all $(TARGET)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)