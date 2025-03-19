# Define the compiler
CC = gcc

# Define the compiler flags
CFLAGS = -Wall -Wextra -std=c11

# Define the target executable
TARGET = osh

# Define the source files
SRCS = main.c

# Define the object files
OBJS = $(SRCS:.c=.o)

# Default rule to build the target
all: $(TARGET)

# Rule to link the object files and create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile the source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

# Add a .PHONY target for non-file targets
.PHONY: all clean
