#ifndef DATABASE_H
#define DATABASE_H

#include "../core/task.h"

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
 * Delete a task by ID.
 * 
 * Returns 0 on success, -1 on error.
 */
int db_delete_task(int id);

/**
 * Get the last error message from the database.
 */
const char* db_get_error(void);

#endif // DATABASE_H
