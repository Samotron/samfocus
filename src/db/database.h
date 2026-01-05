#ifndef DATABASE_H
#define DATABASE_H

#include "../core/task.h"
#include "../core/project.h"

/**
 * Initialize the database connection.
 * Creates the database file if it doesn't exist.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_init(const char* db_path);

/**
 * Create database schema (tables, indices).
 * Safe to call multiple times - uses IF NOT EXISTS.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_create_schema(void);

/**
 * Close the database connection.
 */
void db_close(void);

/**
 * Insert a new task.
 * 
 * Returns the new task ID on success, -1 on error.
 */
int db_insert_task(const char* title, TaskStatus status);

/**
 * Load tasks matching a status filter.
 * 
 * @param tasks Output pointer to array of tasks (caller must free)
 * @param count Output pointer to number of tasks loaded
 * @param status_filter Filter by status, or -1 for all tasks
 * 
 * Returns 0 on success, -1 on error.
 */
int db_load_tasks(Task** tasks, int* count, int status_filter);

/**
 * Update a task's status.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_status(int id, TaskStatus status);

/**
 * Update a task's title.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_title(int id, const char* title);

/**
 * Update a task's notes.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_notes(int id, const char* notes);

/**
 * Update a task's defer date.
 * 
 * @param id Task ID
 * @param defer_at Unix timestamp, or 0 to clear
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_defer_at(int id, time_t defer_at);

/**
 * Update a task's due date.
 * 
 * @param id Task ID
 * @param due_at Unix timestamp, or 0 to clear
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_due_at(int id, time_t due_at);

/**
 * Update a task's flagged status.
 * 
 * @param id Task ID
 * @param flagged 0 = not flagged, 1 = flagged
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_flagged(int id, int flagged);

/**
 * Update a task's order index.
 * 
 * @param id Task ID
 * @param order_index Order position (lower = higher priority)
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_task_order_index(int id, int order_index);

/**
 * Delete a task by ID.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_delete_task(int id);

/**
 * Get the last error message from the database.
 */
const char* db_get_error(void);

// ============================================================================
// Project operations
// ============================================================================

/**
 * Insert a new project.
 * 
 * Returns the new project ID on success, -1 on error.
 */
int db_insert_project(const char* title, ProjectType type);

/**
 * Load all projects.
 * 
 * @param projects Output pointer to array of projects (caller must free)
 * @param count Output pointer to number of projects loaded
 * 
 * Returns 0 on success, -1 on error.
 */
int db_load_projects(Project** projects, int* count);

/**
 * Update a project's title.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_project_title(int id, const char* title);

/**
 * Update a project's type (sequential/parallel).
 * 
 * Returns 0 on success, -1 on error.
 */
int db_update_project_type(int id, ProjectType type);

/**
 * Delete a project by ID.
 * Note: Tasks in this project will have their project_id set to NULL.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_delete_project(int id);

/**
 * Assign a task to a project.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_assign_task_to_project(int task_id, int project_id);

/**
 * Get the first incomplete task in a sequential project.
 * 
 * Returns task ID on success, -1 if no incomplete tasks, -2 on error.
 */
int db_get_first_incomplete_task_in_project(int project_id);

#endif // DATABASE_H
