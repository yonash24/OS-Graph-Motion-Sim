# הגדרות משתנים - הופך את הקובץ לקריא ומקצועי
CC = gcc
CFLAGS = -Wall -std=c11 -I. -I./raylib-5.0_linux_amd64/include
LDFLAGS = -L./raylib-5.0_linux_amd64/lib -lraylib -l:libGL.so.1 -lm -lpthread -ldl -lrt -l:libX11.so.6 -Wl,-rpath,./raylib-5.0_linux_amd64/lib

# שמות קבצי המקור
SRC = dijkstra.c visualization.c main.c
OBJ = $(SRC:.c=.o)

# אבן דרך 1: יוצר קובץ הרצה בשם dijkstra
milestone1: dijkstra.c main.c
	$(CC) $(CFLAGS) dijkstra.c main.c -o dijkstra $(LDFLAGS)

# אבני דרך 2 ו-3: יוצרות קובץ הרצה בשם sim
milestone2: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o sim $(LDFLAGS)

milestone3: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o sim $(LDFLAGS)

milestone4: dijkstra.c visualization.c main5.c
	$(CC) $(CFLAGS) dijkstra.c visualization.c main5.c -o sim $(LDFLAGS)

milestone5: dijkstra.c visualization.c main5.c
	$(CC) $(CFLAGS) dijkstra.c visualization.c main5.c -o sim $(LDFLAGS)

# ניקוי כל קבצי הקימפול - דרישת חובה
clean:
	rm -f dijkstra sim *.o