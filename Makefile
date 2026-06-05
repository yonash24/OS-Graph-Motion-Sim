CC     = gcc
CFLAGS = -Wall -std=c11 -I. -I./raylib-5.0_linux_amd64/include
LDFLAGS = -L./raylib-5.0_linux_amd64/lib -lraylib -l:libGL.so.1 \
          -lm -lpthread -ldl -lrt -l:libX11.so.6 \
          -Wl,-rpath,./raylib-5.0_linux_amd64/lib

SRC = dijkstra.c visualization.c main.c

# Milestone 1 – Dijkstra only (no GUI, no multi-process)
milestone1: dijkstra.c main.c
	$(CC) $(CFLAGS) -DMILESTONE=1 dijkstra.c main.c -o dijkstra $(LDFLAGS)

# Milestones 2-3 – single traveler GUI
milestone2: $(SRC)
	$(CC) $(CFLAGS) -DMILESTONE=2 $(SRC) -o sim $(LDFLAGS)

milestone3: $(SRC)
	$(CC) $(CFLAGS) -DMILESTONE=3 $(SRC) -o sim $(LDFLAGS)

# Milestone 4 – parent computes paths, children sleep
milestone4: $(SRC)
	$(CC) $(CFLAGS) -DMILESTONE=4 $(SRC) -o sim $(LDFLAGS)

# Milestone 5 – autonomous children, IPC via pipe
milestone5: $(SRC)
	$(CC) $(CFLAGS) -DMILESTONE=5 $(SRC) -o sim $(LDFLAGS)

# Milestone 6 – Autonomous children with Node Access Synchronization
milestone6: $(SRC)
	$(CC) $(CFLAGS) -DMILESTONE=6 $(SRC) -o sim $(LDFLAGS)

all: milestone6

clean:
	rm -f dijkstra sim *.o