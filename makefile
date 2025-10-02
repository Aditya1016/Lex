CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
DEBUG_FLAGS = -ggdb
RELEASE_FLAGS = -O2
SDL_FLAGS = $(shell pkg-config --cflags sdl2)
LIBS = $(shell pkg-config --libs sdl2) -lm

# Collect all .c files automatically
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Output executable
TARGET = te

all: debug

# Debug build (default)
debug: CFLAGS += $(DEBUG_FLAGS) $(SDL_FLAGS)
debug: $(TARGET)

# Release build (-O2)
release: CFLAGS += $(RELEASE_FLAGS) $(SDL_FLAGS)
release: $(TARGET)

# Link object files into the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)
