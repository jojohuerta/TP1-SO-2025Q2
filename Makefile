# Compiler, flags
CC = gcc
CFLAGS = -Wall -g -I$(IDIR) -pthread

# Directories
SRC_DIR = ./src
IDIR = ./include

# Targets
TARGET = ChompChamps
VIEW = view
PLAYER = player

# Sources
MASTER_SRCS = $(SRC_DIR)/master.c $(SRC_DIR)/masterShareMemoryManager.c $(SRC_DIR)/masterPlayerManager.c
VIEW_SRCS = $(SRC_DIR)/view.c
PLAYER_SRCS = $(SRC_DIR)/player.c $(SRC_DIR)/playerMovement.c

# Object files
# OBJS = $(SRCS:.c=.o)

# Dependencies
#DEPS = shmConstants.h

all: $(TARGET) $(VIEW) $(PLAYER)

$(TARGET): $(MASTER_SRCS)
	$(CC) -o $@ $^ $(CFLAGS) -lm

$(VIEW): $(VIEW_SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

$(PLAYER): $(PLAYER_SRCS)
	$(CC) -o $@ $^ $(CFLAGS)

shm-clean:
	rm /dev/shm/game_sync /dev/shm/game_state

clean:
	$(shm)
	rm -f $(TARGET) $(VIEW) $(PLAYER) /dev/shm/game_sync /dev/shm/game_state