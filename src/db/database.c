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
        "    project_id INTEGER NULL,"
        "    status INTEGER NOT NULL DEFAULT 0,"
        "    created_at INTEGER NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_tasks_status ON tasks(status);"
        "CREATE INDEX IF NOT EXISTS idx_tasks_project ON tasks(project_id);";
    
    char* err = NULL;
    int rc = sqlite3_exec(db, schema, NULL, NULL, &err);
    
    if (rc != SQLITE_OK) {
        snprintf(error_msg, sizeof(error_msg), 
                 "Failed to create schema: %s", err ? err : "unknown error");
        sqlite3_free(err);
        return -1;
    }
    
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
        sql = "SELECT id, title, project_id, status, created_at FROM tasks "
              "WHERE status = ? ORDER BY created_at DESC;";
    } else {
        sql = "SELECT id, title, project_id, status, created_at FROM tasks "
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
        
        task->project_id = sqlite3_column_int(stmt, 2);
        task->status = (TaskStatus)sqlite3_column_int(stmt, 3);
        task->created_at = (time_t)sqlite3_column_int64(stmt, 4);
        
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
