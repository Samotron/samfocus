#ifndef SIDEBAR_H
#define SIDEBAR_H

#include "../core/project.h"

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
 * @param selected_project_id Output: currently selected project ID (0 for Inbox, -1 for none)
 * @param needs_reload Output: set to 1 if projects need to be reloaded from DB
 */
void sidebar_render(Project* projects, int project_count, int* selected_project_id, int* needs_reload);

/**
 * Cleanup sidebar resources.
 */
void sidebar_cleanup(void);

#endif // SIDEBAR_H
