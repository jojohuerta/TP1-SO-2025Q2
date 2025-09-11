# Compiler, flags
CC = gcc
CFLAGS = -Wall -g -I$(IDIR) -pthread -lm

# Directories
SRC_DIR = ./src
IDIR = ./include

# Targets
TARGET = ChompChamps
VIEW = view
PLAYER = player

# Sources
SRCS = $(SRC_DIR)/shareMemory.c $(SRC_DIR)/utilities.c

# Object files
# OBJS = $(SRCS:.c=.o)

# Dependencies
DEPS = shmConstants.h

all: $(TARGET) $(VIEW) $(PLAYER)

$(TARGET): $(SRC_DIR)/master.c $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

$(VIEW): $(SRC_DIR)/view.c $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

$(PLAYER): $(SRC_DIR)/player.c $(SRCS) $(SRC_DIR)/playerUtils.c
	$(CC) -o $@ $^ $(CFLAGS)

shm-clean:
	rm /dev/shm/game_sync /dev/shm/game_state

clean:
	$(shm)
	rm -f $(TARGET) $(VIEW) $(PLAYER) /dev/shm/game_sync /dev/shm/game_state