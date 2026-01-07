// samfocus-cli.c - Command-line companion tool for SamFocus
// Rewritten with cli.h library

#include <time.h>

#define CLI_IMPLEMENTATION
#include "cli.h"

#include "../core/platform.h"
#include "../db/database.h"
#include "../core/task.h"
#include "../core/project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "2026.1.1"

// ============================================================
// Database Initialization (shared across commands)
// ============================================================

static char* get_db_path(void) {
    const char* data_dir = get_app_data_dir();
    if (!data_dir) return NULL;
    
    if (ensure_dir_exists(data_dir) != 0) return NULL;
    
    static char db_path[512];
    const char* joined = path_join(data_dir, "tasks.db");
    snprintf(db_path, sizeof(db_path), "%s", joined);
    return db_path;
}

static int init_database(cli_ctx *ctx) {
    char* db_path = get_db_path();
    if (!db_path) {
        cli_error(ctx, "Error: Could not determine database path\n");
        return -1;
    }
    
    if (db_init(db_path) != 0) {
        cli_error(ctx, "Error: Could not initialize database: %s\n", db_get_error());
        return -1;
    }
    
    if (db_create_schema() != 0) {
        cli_error(ctx, "Error: Could not create schema: %s\n", db_get_error());
        db_close();
        return -1;
    }
    
    return 0;
}

// ============================================================
// Helper Functions
// ============================================================

static void format_date(time_t timestamp, char* buffer, size_t size) {
    if (timestamp == 0) {
        snprintf(buffer, size, "-");
        return;
    }
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d", tm_info);
}

static const char* format_status(TaskStatus status) {
    switch (status) {
        case TASK_STATUS_INBOX: return "INBOX";
        case TASK_STATUS_ACTIVE: return "ACTIVE";
        case TASK_STATUS_DONE: return "DONE";
        default: return "UNKNOWN";
    }
}

static const char* format_recurrence(RecurrencePattern recur, int interval) {
    if (recur == RECUR_NONE) return "-";
    
    static char buffer[64];
    const char* names[] = {"", "daily", "weekly", "monthly", "yearly"};
    
    if (interval == 1) {
        snprintf(buffer, sizeof(buffer), "%s", names[recur]);
    } else {
        snprintf(buffer, sizeof(buffer), "every %d %s", interval, names[recur]);
    }
    return buffer;
}

// ============================================================
// Command Handlers
// ============================================================

static int cmd_list(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    int filter = -1;
    if (cli_opt_bool(c, "--inbox")) filter = TASK_STATUS_INBOX;
    else if (cli_opt_bool(c, "--active")) filter = TASK_STATUS_ACTIVE;
    else if (cli_opt_bool(c, "--done")) filter = TASK_STATUS_DONE;
    
    Task* tasks = NULL;
    int count = 0;
    
    if (db_load_tasks(&tasks, &count, filter) != 0) {
        cli_error(c, "Error loading tasks: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    cli_print(c, "%-4s %-10s %-40s %-12s %-12s %-3s %-15s\n", 
              "ID", "STATUS", "TITLE", "DEFER", "DUE", "FLG", "RECURRENCE");
    cli_print(c, "--------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        Task* t = &tasks[i];
        char defer_str[16], due_str[16];
        format_date(t->defer_at, defer_str, sizeof(defer_str));
        format_date(t->due_at, due_str, sizeof(due_str));
        
        cli_print(c, "%-4d %-10s %-40s %-12s %-12s %-3s %-15s\n",
                  t->id, format_status(t->status), t->title,
                  defer_str, due_str, t->flagged ? "YES" : "NO",
                  format_recurrence(t->recurrence, t->recurrence_interval));
    }
    
    cli_print(c, "\nTotal: %d task(s)\n", count);
    
    free(tasks);
    db_close();
    return 0;
}

static int cmd_add(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    const char* title = cli_arg(c, 0);
    if (!title) {
        cli_error(c, "Error: Task title is required\n");
        db_close();
        return 1;
    }
    
    int task_id = db_insert_task(title, TASK_STATUS_INBOX);
    if (task_id < 0) {
        cli_error(c, "Error adding task: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    // Handle optional dates
    const char* defer_str = cli_opt_str(c, "--defer");
    if (defer_str) {
        struct tm tm = {0};
        if (sscanf(defer_str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            time_t defer_at = mktime(&tm);
            db_update_task_defer_at(task_id, defer_at);
        }
    }
    
    const char* due_str = cli_opt_str(c, "--due");
    if (due_str) {
        struct tm tm = {0};
        if (sscanf(due_str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            time_t due_at = mktime(&tm);
            db_update_task_due_at(task_id, due_at);
        }
    }
    
    if (cli_opt_bool(c, "--flag")) {
        db_update_task_flagged(task_id, 1);
    }
    
    cli_print(c, "Task added successfully (ID: %d)\n", task_id);
    db_close();
    return 0;
}

static int cmd_complete(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    const char* id_str = cli_arg(c, 0);
    if (!id_str) {
        cli_error(c, "Error: Task ID is required\n");
        db_close();
        return 1;
    }
    
    int task_id = atoi(id_str);
    if (task_id <= 0) {
        cli_error(c, "Error: Invalid task ID\n");
        db_close();
        return 1;
    }
    
    // Load task to check recurrence
    Task* tasks = NULL;
    int count = 0;
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        cli_error(c, "Error loading tasks: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    Task* task = NULL;
    for (int i = 0; i < count; i++) {
        if (tasks[i].id == task_id) {
            task = &tasks[i];
            break;
        }
    }
    
    if (!task) {
        cli_error(c, "Error: Task not found\n");
        free(tasks);
        db_close();
        return 1;
    }
    
    if (db_update_task_status(task_id, TASK_STATUS_DONE) != 0) {
        cli_error(c, "Error completing task: %s\n", db_get_error());
        free(tasks);
        db_close();
        return 1;
    }
    
    // Handle recurring tasks
    if (task->recurrence != RECUR_NONE) {
        int new_id = db_create_recurring_instance(task);
        if (new_id > 0) {
            cli_print(c, "Task completed and next instance created (ID: %d)\n", new_id);
        } else {
            cli_print(c, "Task completed (warning: could not create next instance)\n");
        }
    } else {
        cli_print(c, "Task completed successfully\n");
    }
    
    free(tasks);
    db_close();
    return 0;
}

static int cmd_delete(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    const char* id_str = cli_arg(c, 0);
    if (!id_str) {
        cli_error(c, "Error: Task ID is required\n");
        db_close();
        return 1;
    }
    
    int task_id = atoi(id_str);
    if (task_id <= 0) {
        cli_error(c, "Error: Invalid task ID\n");
        db_close();
        return 1;
    }
    
    if (db_delete_task(task_id) != 0) {
        cli_error(c, "Error deleting task: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    cli_print(c, "Task deleted successfully\n");
    db_close();
    return 0;
}

static int cmd_show(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    const char* id_str = cli_arg(c, 0);
    if (!id_str) {
        cli_error(c, "Error: Task ID is required\n");
        db_close();
        return 1;
    }
    
    int task_id = atoi(id_str);
    if (task_id <= 0) {
        cli_error(c, "Error: Invalid task ID\n");
        db_close();
        return 1;
    }
    
    Task* tasks = NULL;
    int count = 0;
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        cli_error(c, "Error loading tasks: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    Task* task = NULL;
    for (int i = 0; i < count; i++) {
        if (tasks[i].id == task_id) {
            task = &tasks[i];
            break;
        }
    }
    
    if (!task) {
        cli_error(c, "Error: Task not found\n");
        free(tasks);
        db_close();
        return 1;
    }
    
    char defer_str[32], due_str[32], created_str[32], modified_str[32];
    format_date(task->defer_at, defer_str, sizeof(defer_str));
    format_date(task->due_at, due_str, sizeof(due_str));
    format_date(task->created_at, created_str, sizeof(created_str));
    format_date(task->modified_at, modified_str, sizeof(modified_str));
    
    cli_print(c, "\n");
    cli_print(c, "Task ID:       %d\n", task->id);
    cli_print(c, "Title:         %s\n", task->title);
    cli_print(c, "Status:        %s\n", format_status(task->status));
    cli_print(c, "Project ID:    %d\n", task->project_id);
    cli_print(c, "Flagged:       %s\n", task->flagged ? "YES" : "NO");
    cli_print(c, "Defer Date:    %s\n", defer_str);
    cli_print(c, "Due Date:      %s\n", due_str);
    cli_print(c, "Created:       %s\n", created_str);
    cli_print(c, "Modified:      %s\n", modified_str);
    cli_print(c, "Recurrence:    %s\n", format_recurrence(task->recurrence, task->recurrence_interval));
    cli_print(c, "Order Index:   %d\n", task->order_index);
    
    if (task->notes[0] != '\0') {
        cli_print(c, "\nNotes:\n%s\n", task->notes);
    }
    
    cli_print(c, "\n");
    
    free(tasks);
    db_close();
    return 0;
}

static int cmd_projects(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    Project* projects = NULL;
    int count = 0;
    
    if (db_load_projects(&projects, &count) != 0) {
        cli_error(c, "Error loading projects: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    cli_print(c, "%-4s %-12s %-40s\n", "ID", "TYPE", "TITLE");
    cli_print(c, "----------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        Project* p = &projects[i];
        const char* type_str = (p->type == PROJECT_TYPE_SEQUENTIAL) ? "SEQUENTIAL" : "PARALLEL";
        cli_print(c, "%-4d %-12s %-40s\n", p->id, type_str, p->title);
    }
    
    cli_print(c, "\nTotal: %d project(s)\n", count);
    
    free(projects);
    db_close();
    return 0;
}

static int cmd_today(cli_ctx *c) {
    if (init_database(c) != 0) return 1;
    
    Task* tasks = NULL;
    int count = 0;
    
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        cli_error(c, "Error loading tasks: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);
    
    cli_print(c, "Tasks for today:\n");
    cli_print(c, "%-4s %-40s %-12s %-3s\n", "ID", "TITLE", "DUE", "FLG");
    cli_print(c, "----------------------------------------------------------------\n");
    
    int today_count = 0;
    for (int i = 0; i < count; i++) {
        Task* t = &tasks[i];
        
        if (t->status == TASK_STATUS_DONE) continue;
        
        bool show = false;
        if (t->defer_at == 0) {
            show = true;
        } else {
            struct tm* defer_tm = localtime(&t->defer_at);
            if (defer_tm->tm_year < now_tm->tm_year ||
                (defer_tm->tm_year == now_tm->tm_year && defer_tm->tm_yday <= now_tm->tm_yday)) {
                show = true;
            }
        }
        
        if (show) {
            char due_str[16];
            format_date(t->due_at, due_str, sizeof(due_str));
            cli_print(c, "%-4d %-40s %-12s %-3s\n",
                      t->id, t->title, due_str, t->flagged ? "YES" : "NO");
            today_count++;
        }
    }
    
    cli_print(c, "\nTotal: %d task(s) available today\n", today_count);
    
    free(tasks);
    db_close();
    return 0;
}

// ============================================================
// Application Definition
// ============================================================

int main(int argc, char** argv) {
    cli_app app = {
        .name = "samfocus-cli",
        .version = VERSION,
        .description = "Command-line task management for SamFocus",
        .author = "Sam",
        
        CLI_COMMANDS(
            {
                .route = "list",
                .summary = "List all tasks",
                .handler = cmd_list,
                .options = (cli_option[]){
                    { .long_name = "--inbox", .short_name = "-i", .type = CLI_TYPE_BOOL, .description = "Show only inbox tasks" },
                    { .long_name = "--active", .short_name = "-a", .type = CLI_TYPE_BOOL, .description = "Show only active tasks" },
                    { .long_name = "--done", .short_name = "-d", .type = CLI_TYPE_BOOL, .description = "Show only completed tasks" },
                },
                .options_count = 3,
            },
            {
                .route = "add",
                .summary = "Add a new task",
                .handler = cmd_add,
                .args = (cli_arg_def[]){
                    { .name = "title", .description = "Task title", .required = true },
                },
                .args_count = 1,
                .options = (cli_option[]){
                    { .long_name = "--defer", .short_name = "-D", .type = CLI_TYPE_STRING, .description = "Defer until date (YYYY-MM-DD)" },
                    { .long_name = "--due", .short_name = "-u", .type = CLI_TYPE_STRING, .description = "Due date (YYYY-MM-DD)" },
                    { .long_name = "--flag", .short_name = "-f", .type = CLI_TYPE_BOOL, .description = "Mark task as flagged" },
                },
                .options_count = 3,
            },
            {
                .route = "complete",
                .aliases = "done",
                .summary = "Mark task as complete",
                .handler = cmd_complete,
                .args = (cli_arg_def[]){
                    { .name = "id", .description = "Task ID", .required = true },
                },
                .args_count = 1,
            },
            {
                .route = "delete",
                .aliases = "rm",
                .summary = "Delete a task",
                .handler = cmd_delete,
                .args = (cli_arg_def[]){
                    { .name = "id", .description = "Task ID", .required = true },
                },
                .args_count = 1,
            },
            {
                .route = "show",
                .summary = "Show task details",
                .handler = cmd_show,
                .args = (cli_arg_def[]){
                    { .name = "id", .description = "Task ID", .required = true },
                },
                .args_count = 1,
            },
            {
                .route = "projects",
                .summary = "List all projects",
                .handler = cmd_projects,
            },
            {
                .route = "today",
                .summary = "Show today's available tasks",
                .handler = cmd_today,
            },
        ),
        
        .groups = (cli_command_group[]){
            { .name = "TASK MANAGEMENT", .description = "Core task operations", .start_idx = 0, .count = 5 },
            { .name = "ORGANIZATION", .description = "Projects and views", .start_idx = 5, .count = 2 },
        },
        .groups_count = 2,
    };
    
    return cli_run(&app, argc, argv);
}
