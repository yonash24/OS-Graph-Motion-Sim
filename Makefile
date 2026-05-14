CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.
LDFLAGS = -L. -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC = dijkstra.c visualization.c main.c
OBJ = $(SRC:.c=.o)
TARGET_MS1 = dijkstra
TARGET_SIM = sim

.PHONY: all clean milestone1 milestone2 milestone3 milestone4

all: milestone4

milestone1: dijkstra.c main.c
	$(CC) $(CFLAGS) dijkstra.c main.c -o $(TARGET_MS1) $(LDFLAGS)

milestone2 milestone3 milestone4: $(TARGET_SIM)

$(TARGET_SIM): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET_SIM) $(LDFLAGS)

%.o: %.c main.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET_MS1) $(TARGET_SIM) *.o
