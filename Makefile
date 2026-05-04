CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Wno-unused-parameter -I./raylib-5.0_linux_amd64/include
RAYLIB  = ./raylib-5.0_linux_amd64/lib/libraylib.a
LIBS    = $(RAYLIB) -lm -ldl -lpthread

SIM     = sim
DIJK    = dijkstra

.PHONY: all milestone1 milestone2 milestone3 clean

all: milestone3

milestone1: $(DIJK)

milestone2: $(SIM)

milestone3: $(SIM)

$(DIJK): dijkstra.c
	$(CC) $(CFLAGS) -o $@ $^

$(SIM): main.c graph_visual.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(SIM) $(DIJK)
