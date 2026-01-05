#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "../core/task.h"
#include "../core/project.h"
#include "../core/context.h"

/**
 * Initialize the launcher system
 */
void launcher_init(void);

/**
 * Show the launcher window
 */
void launcher_show(void);

/**
 * Hide the launcher window
 */
void launcher_hide(void);

/**
 * Check if launcher is visible
 */
int launcher_is_visible(void);

/**
 * Render the launcher UI
 * 
 * @param tasks Array of tasks for searching
 * @param task_count Number of tasks
 * @param projects Array of projects
 * @param project_count Number of projects
 * @param contexts Array of contexts
 * @param context_count Number of contexts
 * @param needs_reload Output flag to signal main app to reload
 */
void launcher_render(Task* tasks, int task_count, 
                     Project* projects, int project_count,
                     Context* contexts, int context_count,
                     int* needs_reload);

#endif // LAUNCHER_H
