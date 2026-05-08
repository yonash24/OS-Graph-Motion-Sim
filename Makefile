# הגדרות משתנים - הופך את הקובץ לקריא ומקצועי
CC = gcc
CFLAGS = -Wall -std=c11 -I.
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

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

# ניקוי כל קבצי הקימפול - דרישת חובה
clean:
	rm -f dijkstra sim *.o