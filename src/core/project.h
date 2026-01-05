#ifndef PROJECT_H
#define PROJECT_H

#include <time.h>

// Project type enumeration
typedef enum {
    PROJECT_TYPE_SEQUENTIAL = 0,  // Only first incomplete task is active
    PROJECT_TYPE_PARALLEL = 1     // All tasks are active (future)
} ProjectType;

// Project structure
typedef struct {
    int id;
    char title[256];
    ProjectType type;
    time_t created_at;
} Project;

#endif // PROJECT_H
