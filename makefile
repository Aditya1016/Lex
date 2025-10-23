CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
DEBUG_FLAGS = -ggdb
RELEASE_FLAGS = -O2
SDL_FLAGS = $(shell pkg-config --cflags sdl2)

# Add gdi32 (needed on Windows for OpenGL context)
LIBS = $(shell pkg-config --libs sdl2) -lglew32 -lopengl32 -lgdi32 -lm

SRC_DIR = src
BUILD_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = te

all: debug

debug: CFLAGS += $(DEBUG_FLAGS) $(SDL_FLAGS)
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS) $(SDL_FLAGS)
release: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
