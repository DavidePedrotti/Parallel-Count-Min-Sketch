CC = mpicc
CFLAGS = -g -Wall -std=c99

SRCS = main.c
TARGET = main # Executable name

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
        $(CC) $(CFLAGS) -o $@ $^