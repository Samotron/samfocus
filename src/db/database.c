#include "database.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static sqlite3* db = NULL;
static char error_msg[512] = {0};

static void set_error(const char* msg) {
    snprintf(error_msg, sizeof(error_msg), "%s", msg);
}

const char* db_get_error(void) {
    return error_msg;
}

int db_init(const char* db_path) {
    if (db != NULL) {
        set_error("Database already initialized");
        return -1;
    }
    
    int rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Cannot open database: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        db = NULL;
        return -1;
    }
    
    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    
    return 0;
}

int db_create_schema(void) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* schema = 
        "CREATE TABLE IF NOT EXISTS tasks ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title TEXT NOT NULL,"
        "    notes TEXT DEFAULT '',"
        "    project_id INTEGER NULL,"
        "    status INTEGER NOT NULL DEFAULT 0,"
        "    created_at INTEGER NOT NULL,"
        "    defer_at INTEGER DEFAULT 0,"
        "    due_at INTEGER DEFAULT 0,"
        "    flagged INTEGER DEFAULT 0,"
        "    FOREIGN KEY (project_id) REFERENCES projects(id) ON DELETE SET NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);"
        "CREATE INDEX IF NOT EXISTS idx_tasks_project ON tasks(project_id);"
        "CREATE INDEX IF NOT EXISTS idx_tasks_flagged ON tasks(flagged);"
        ""
        "CREATE TABLE IF NOT EXISTS projects ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title TEXT NOT NULL,"
        "    type INTEGER NOT NULL DEFAULT 0,"
        "    created_at INTEGER NOT NULL"
        ");";
    
    char* err = NULL;
    int rc = sqlite3_exec(db, schema, NULL, NULL, &err);
    
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to create schema: %s", err ? err : "unknown error");
        sqlite3_free(err);
        return -1;
    }
    
    // Migrate existing databases - add flagged column if it doesn't exist
    // This will fail silently if the column already exists, which is fine
    sqlite3_exec(db, "ALTER TABLE tasks ADD COLUMN flagged INTEGER DEFAULT 0;", NULL, NULL, NULL);
    
    return 0;
}

void db_close(void) {
    if (db != NULL) {
        sqlite3_close(db);
        db = NULL;
    }
}

int db_insert_task(const char* title, TaskStatus status) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (title == NULL || title[0] == '\0') {
        set_error("Task title cannot be empty");
        return -1;
    }
    
    const char* sql = "INSERT INTO tasks (title, status, created_at) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, status);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)time(NULL));
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to insert task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_load_tasks(Task** tasks, int* count, int status_filter) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (tasks == NULL || count == NULL) {
        set_error("Invalid parameters");
        return -1;
    }
    
    *tasks = NULL;
    *count = 0;
    
    // Build query based on filter
    const char* sql;
    if (status_filter >= 0) {
        sql = "SELECT id, title, notes, project_id, status, created_at, defer_at, due_at, flagged FROM tasks "
              "WHERE status = ? ORDER BY created_at DESC;";
    } else {
        sql = "SELECT id, title, notes, project_id, status, created_at, defer_at, due_at, flagged FROM tasks "
              "ORDER BY created_at DESC;";
    }
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    if (status_filter >= 0) {
        sqlite3_bind_int(stmt, 1, status_filter);
    }
    
    // Count results first
    int capacity = 16;
    *tasks = (Task*)malloc(sizeof(Task) * capacity);
    if (*tasks == NULL) {
        set_error("Out of memory");
        sqlite3_finalize(stmt);
        return -1;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            Task* new_tasks = (Task*)realloc(*tasks, sizeof(Task) * capacity);
            if (new_tasks == NULL) {
                set_error("Out of memory");
                free(*tasks);
                *tasks = NULL;
                *count = 0;
                sqlite3_finalize(stmt);
                return -1;
            }
            *tasks = new_tasks;
        }
        
        Task* task = &(*tasks)[*count];
        task->id = sqlite3_column_int(stmt, 0);
        
        const char* title = (const char*)sqlite3_column_text(stmt, 1);
        strncpy(task->title, title ? title : "", sizeof(task->title) - 1);
        task->title[sizeof(task->title) - 1] = '\0';
        
        const char* notes = (const char*)sqlite3_column_text(stmt, 2);
        strncpy(task->notes, notes ? notes : "", sizeof(task->notes) - 1);
        task->notes[sizeof(task->notes) - 1] = '\0';
        
        task->project_id = sqlite3_column_int(stmt, 3);
        task->status = (TaskStatus)sqlite3_column_int(stmt, 4);
        task->created_at = (time_t)sqlite3_column_int64(stmt, 5);
        task->defer_at = (time_t)sqlite3_column_int64(stmt, 6);
        task->due_at = (time_t)sqlite3_column_int64(stmt, 7);
        task->flagged = sqlite3_column_int(stmt, 8);
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Error reading tasks: %s", sqlite3_errmsg(db));
        free(*tasks);
        *tasks = NULL;
        *count = 0;
        return -1;
    }
    
    return 0;
}

int db_update_task_status(int id, TaskStatus status) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET status = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_task_title(int id, const char* title) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (title == NULL || title[0] == '\0') {
        set_error("Task title cannot be empty");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET title = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_task_notes(int id, const char* notes) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET notes = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, notes ? notes : "", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task notes: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_task_defer_at(int id, time_t defer_at) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET defer_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)defer_at);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_task_due_at(int id, time_t due_at) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET due_at = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)due_at);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_task_flagged(int id, int flagged) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET flagged = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, flagged ? 1 : 0);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update task flagged status: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_delete_task(int id) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "DELETE FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to delete task: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

// ============================================================================
// Project operations
// ============================================================================

int db_insert_project(const char* title, ProjectType type) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (title == NULL || title[0] == '\0') {
        set_error("Project title cannot be empty");
        return -1;
    }
    
    const char* sql = "INSERT INTO projects (title, type, created_at) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, type);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)time(NULL));
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to insert project: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return (int)sqlite3_last_insert_rowid(db);
}

int db_load_projects(Project** projects, int* count) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (projects == NULL || count == NULL) {
        set_error("Invalid parameters");
        return -1;
    }
    
    *projects = NULL;
    *count = 0;
    
    const char* sql = "SELECT id, title, type, created_at FROM projects "
                     "ORDER BY created_at ASC;";
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    int capacity = 16;
    *projects = (Project*)malloc(sizeof(Project) * capacity);
    if (*projects == NULL) {
        set_error("Out of memory");
        sqlite3_finalize(stmt);
        return -1;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            Project* new_projects = (Project*)realloc(*projects, sizeof(Project) * capacity);
            if (new_projects == NULL) {
                set_error("Out of memory");
                free(*projects);
                *projects = NULL;
                *count = 0;
                sqlite3_finalize(stmt);
                return -1;
            }
            *projects = new_projects;
        }
        
        Project* project = &(*projects)[*count];
        project->id = sqlite3_column_int(stmt, 0);
        
        const char* title = (const char*)sqlite3_column_text(stmt, 1);
        strncpy(project->title, title ? title : "", sizeof(project->title) - 1);
        project->title[sizeof(project->title) - 1] = '\0';
        
        project->type = (ProjectType)sqlite3_column_int(stmt, 2);
        project->created_at = (time_t)sqlite3_column_int64(stmt, 3);
        
        (*count)++;
    }
    
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Error reading projects: %s", sqlite3_errmsg(db));
        free(*projects);
        *projects = NULL;
        *count = 0;
        return -1;
    }
    
    return 0;
}

int db_update_project_title(int id, const char* title) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    if (title == NULL || title[0] == '\0') {
        set_error("Project title cannot be empty");
        return -1;
    }
    
    const char* sql = "UPDATE projects SET title = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update project: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_update_project_type(int id, ProjectType type) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE projects SET type = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, type);
    sqlite3_bind_int(stmt, 2, id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to update project type: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_delete_project(int id) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    // First, unassign all tasks from this project
    const char* unassign_sql = "UPDATE tasks SET project_id = NULL WHERE project_id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, unassign_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to unassign tasks: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    // Now delete the project
    const char* delete_sql = "DELETE FROM projects WHERE id = ?;";
    rc = sqlite3_prepare_v2(db, delete_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to delete project: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_assign_task_to_project(int task_id, int project_id) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    
    const char* sql = "UPDATE tasks SET project_id = ? WHERE id = ?;";
    sqlite3_stmt* stmt = NULL;
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    if (project_id == 0) {
        sqlite3_bind_null(stmt, 1);
    } else {
        sqlite3_bind_int(stmt, 1, project_id);
    }
    sqlite3_bind_int(stmt, 2, task_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to assign task to project: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    return 0;
}

int db_get_first_incomplete_task_in_project(int project_id) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -2;
    }
    
    const char* sql = "SELECT id FROM tasks "
                     "WHERE project_id = ? AND status != ? "
                     "ORDER BY created_at ASC LIMIT 1;";
    
    sqlite3_stmt* stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to prepare statement: %s", sqlite3_errmsg(db));
        return -2;
    }
    
    sqlite3_bind_int(stmt, 1, project_id);
    sqlite3_bind_int(stmt, 2, TASK_STATUS_DONE);
    
    rc = sqlite3_step(stmt);
    
    int task_id = -1;
    if (rc == SQLITE_ROW) {
        task_id = sqlite3_column_int(stmt, 0);
    } else if (rc != SQLITE_DONE) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Error querying first task: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -2;
    }
    
    sqlite3_finalize(stmt);
    return task_id;
}
