#ifndef IPC_H
#define IPC_H

#include <sys/types.h>

// הגדרת מצבי הנוסע עבור דיווח לאב ועדכון ה-GUI
typedef enum {
    STATUS_DRIVING,            // הנוסע בתנועה על קשת
    STATUS_WAITING_FOR_NODE,   // הנוסע הגיע לצומת וממתין מחוץ לו
    STATUS_INSIDE_NODE,        // הנוסע קיבל אישור, נכנס לצומת
    STATUS_ARRIVED_DEST,       // הנוסע הגיע ליעד הסופי
    STATUS_SCHEDULE_REQUEST    // M7: בקשת כניסה לצומת (תור תזמון)
} TravelerStatus;

typedef struct {
    pid_t child_pid;
    int   current_node;
    int   next_node;
    int   status;
    int   remaining_cost;      // M7/SJF: משקל מסלול שנותר
} IPC_Message;

#endif