#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    Graph graph;
    Traveler* travelers = NULL;
    int numTravelers = 0;

    // טעינת גרף וחישוב מסלולים לכל הנוסעים
    if (!loadGraphExtended(argv[1], &graph, &travelers, &numTravelers)) {
        return 1;
    }

    // יצירת תהליכי הבנים
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            return 1;
        }

        if (pid == 0) {
            // --- תהליך הבן ---
            printf("[%d] started\n", getpid());
            fflush(stdout); // מוודא שההדפסה תצא מיד
            while (1) {
                pause(); // מחכה לסיגנלים (כמו SIGTERM) מהאבא
            }
            exit(0);
        } else {
            // --- תהליך האב ---
            travelers[i].pid = pid;
        }
    }

    // הפעלת ה-GUI (תהליך האב)
    visualizeGraph(&graph, travelers, numTravelers);

    // ניקוי בסיום
    for (int i = 0; i < numTravelers; i++) {
        waitpid(travelers[i].pid, NULL, 0);
        if (travelers[i].path) free(travelers[i].path);
    }
    
    if (travelers) free(travelers);
    freeGraph(&graph);
    
    return 0;
}