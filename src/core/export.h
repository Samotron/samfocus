#ifndef EXPORT_H
#define EXPORT_H

#include "task.h"
#include "project.h"

typedef enum {
    EXPORT_FORMAT_TEXT,
    EXPORT_FORMAT_MARKDOWN,
    EXPORT_FORMAT_CSV
} ExportFormat;

/**
 * Export all tasks to a file in the specified format.
 * 
 * @param filepath Path to output file
 * @param format Export format
 * @param tasks Array of tasks
 * @param task_count Number of tasks
 * @param projects Array of projects (optional, can be NULL)
 * @param project_count Number of projects
 * 
 * Returns 0 on success, -1 on error.
 */
int export_tasks(const char* filepath, ExportFormat format,
                 Task* tasks, int task_count,
                 Project* projects, int project_count);

/**
 * Create a backup of the entire database.
 * Creates a timestamped .db.bak file.
 * 
 * @param db_path Path to the database file
 * 
 * Returns 0 on success, -1 on error.
 */
int export_create_backup(const char* db_path);

/**
 * Get the last error message from export operations.
 */
const char* export_get_error(void);

/**
 * Get the default export directory.
 * Returns a static buffer with the path.
 */
const char* export_get_default_dir(void);

#endif // EXPORT_H
