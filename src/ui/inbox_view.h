#ifndef INBOX_VIEW_H
#define INBOX_VIEW_H

#include "../core/task.h"
#include "../core/project.h"
#include "../core/context.h"

/**
 * Initialize the inbox view.
 * Call this once at startup.
 */
void inbox_view_init(void);

/**
 * Render the inbox view.
 * Call this every frame.
 * 
 * @param tasks Array of tasks to display
 * @param task_count Number of tasks in the array
 * @param projects Array of all projects (for assignment dropdown)
 * @param project_count Number of projects
 * @param contexts Array of all contexts (for context assignment)
 * @param context_count Number of contexts
 * @param selected_project_id Currently selected project (0 for Inbox)
 * @param needs_reload Output: set to 1 if tasks need to be reloaded from DB
 */
void inbox_view_render(Task* tasks, int task_count, Project* projects, int project_count,
                       Context* contexts, int context_count, int selected_project_id, int* needs_reload);

/**
 * Cleanup inbox view resources.
 */
void inbox_view_cleanup(void);

#endif // INBOX_VIEW_H
