#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "../core/project.h"
#include "../core/context.h"
#include "../core/task.h"

/**
 * Initialize the sidebar.
 * Call this once at startup.
 */
void sidebar_init(void);

/**
 * Render the sidebar.
 * Call this every frame.
 * 
 * @param projects Array of projects to display
 * @param project_count Number of projects in the array
 * @param contexts Array of contexts to display
 * @param context_count Number of contexts in the array
 * @param tasks Array of all tasks (for counting)
 * @param task_count Number of tasks in the array
 * @param selected_project_id Output: currently selected project ID
 * @param selected_context_id Output: currently selected context ID (0 for no filter)
 * @param needs_reload Output: set to 1 if projects/contexts need to be reloaded from DB
 */
void sidebar_render(Project* projects, int project_count, Context* contexts, int context_count,
                   Task* tasks, int task_count,
                   int* selected_project_id, int* selected_context_id, int* needs_reload);

/**
 * Cleanup sidebar resources.
 */
void sidebar_cleanup(void);

#endif // SIDEBAR_H
