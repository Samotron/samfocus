#ifndef TASK_H
#define TASK_H

#include <time.h>

// Task status enumeration
typedef enum {
    TASK_STATUS_INBOX = 0,
    TASK_STATUS_ACTIVE = 1,
    TASK_STATUS_DONE = 2
} TaskStatus;

// Recurrence pattern enumeration
typedef enum {
    RECUR_NONE = 0,
    RECUR_DAILY = 1,
    RECUR_WEEKLY = 2,
    RECUR_MONTHLY = 3,
    RECUR_YEARLY = 4
} RecurrencePattern;

// Task structure
typedef struct {
    int id;
    char title[256];
    char notes[1024];  // Task notes/description
    int project_id;  // NULL/0 if not assigned to a project
    TaskStatus status;
    time_t created_at;
    time_t modified_at;  // Last modification timestamp
    time_t defer_at;  // 0 if not deferred (task available immediately)
    time_t due_at;    // 0 if no due date
    int flagged;      // 0 = not flagged, 1 = flagged/starred
    int order_index;  // Manual ordering within list (lower = higher priority)
    RecurrencePattern recurrence;  // Recurrence pattern
    int recurrence_interval;  // Interval for recurrence (e.g., every 2 days)
} Task;

#endif // TASK_H
