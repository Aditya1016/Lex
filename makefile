CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -ggdb $(shell pkg-config --cflags sdl2)
LIBS = $(shell pkg-config --libs sdl2) -lm

# Collect all .c files automatically
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = te

all: $(TARGET)

# Link object files into the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)
