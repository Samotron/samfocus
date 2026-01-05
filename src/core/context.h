#ifndef CONTEXT_H
#define CONTEXT_H

#include <time.h>

// Context structure (for tags like @home, @computer, @errands)
typedef struct {
    int id;
    char name[64];       // Context name (e.g., "home", "computer", "phone")
    char color[8];       // Hex color for display (e.g., "#FF5733")
    time_t created_at;
} Context;

#endif // CONTEXT_H
