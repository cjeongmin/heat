CC = gcc
SRC_DIR = ./src
OBJ_DIR = ./build
TARGET = heat
SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)
OBJECTS = $(patsubst %.o,$(OBJ_DIR)/%.o,$(OBJS))
DEPS = $(OBJECTS:.o=.d)

CFLAGS = -Wall -Wextra -pedantic -std=c11 -MMD -MP

all: $(TARGET)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET) : $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

-include $(DEPS)
