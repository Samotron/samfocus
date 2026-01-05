#ifndef TASK_H
#define TASK_H

#include <time.h>

// Task status enumeration
typedef enum {
    TASK_STATUS_INBOX = 0,
    TASK_STATUS_ACTIVE = 1,
    TASK_STATUS_DONE = 2
} TaskStatus;

// Task structure
typedef struct {
    int id;
    char title[256];
    int project_id;  // NULL/0 if not assigned to a project
    TaskStatus status;
    time_t created_at;
} Task;

#endif // TASK_H
