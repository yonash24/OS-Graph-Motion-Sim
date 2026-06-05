#ifndef IPC_H
#define IPC_H

#include <sys/types.h>

// הגדרת מצבי הנוסע עבור דיווח לאב ועדכון ה-GUI
typedef enum {
    STATUS_DRIVING,          // הנוסע בתנועה על קשת
    STATUS_WAITING_FOR_NODE, // הנוסע הגיע לצומת וממתין מחוץ לו (תקוע במנעול)
    STATUS_INSIDE_NODE,      // הנוסע קיבל אישור, נכנס לצומת וממתין שנייה
    STATUS_ARRIVED_DEST      // הנוסע הגיע ליעד הסופי שלו
} TravelerStatus;

typedef struct {
    pid_t child_pid;
    int current_node;
    int next_node;
    int status;              // ישתמש בערכים מתוך TravelerStatus
} IPC_Message;

#endif