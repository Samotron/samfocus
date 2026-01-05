#ifndef INBOX_VIEW_H
#define INBOX_VIEW_H

#include "../core/task.h"

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
 * @param needs_reload Output: set to 1 if tasks need to be reloaded from DB
 */
void inbox_view_render(Task* tasks, int task_count, int* needs_reload);

/**
 * Cleanup inbox view resources.
 */
void inbox_view_cleanup(void);

#endif // INBOX_VIEW_H
