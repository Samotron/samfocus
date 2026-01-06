// samfocus-cli.c - Command-line companion tool for SamFocus
// Allows quick task management from the terminal

#include "../core/platform.h"
#include "../db/database.h"
#include "../core/task.h"
#include "../core/project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define VERSION "1.0.0"

// Helper to get database path (same as main app)
static char* get_db_path(void) {
    const char* data_dir = get_app_data_dir();
    if (!data_dir) {
        fprintf(stderr, "Error: Could not determine application data directory\n");
        return NULL;
    }
    
    // Ensure directory exists
    if (ensure_dir_exists(data_dir) != 0) {
        fprintf(stderr, "Error: Could not create directory: %s\n", data_dir);
        return NULL;
    }
    
    static char db_path[512];
    const char* joined = path_join(data_dir, "tasks.db");
    snprintf(db_path, sizeof(db_path), "%s", joined);
    
    return db_path;
}

// Helper to format date
static void format_date(time_t timestamp, char* buffer, size_t size) {
    if (timestamp == 0) {
        snprintf(buffer, size, "-");
        return;
    }
    
    struct tm* tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d", tm_info);
}

// Helper to format task status
static const char* format_status(TaskStatus status) {
    switch (status) {
        case TASK_STATUS_INBOX: return "INBOX";
        case TASK_STATUS_ACTIVE: return "ACTIVE";
        case TASK_STATUS_DONE: return "DONE";
        default: return "UNKNOWN";
    }
}

// Helper to format recurrence
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

// Command: list tasks
static int cmd_list(int argc, char** argv) {
    int filter = -1;  // -1 = all
    
    // Parse options
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--inbox") == 0) {
            filter = TASK_STATUS_INBOX;
        } else if (strcmp(argv[i], "--active") == 0) {
            filter = TASK_STATUS_ACTIVE;
        } else if (strcmp(argv[i], "--done") == 0) {
            filter = TASK_STATUS_DONE;
        }
    }
    
    Task* tasks = NULL;
    int count = 0;
    
    if (db_load_tasks(&tasks, &count, filter) != 0) {
        fprintf(stderr, "Error loading tasks: %s\n", db_get_error());
        return 1;
    }
    
    printf("%-4s %-10s %-40s %-12s %-12s %-3s %-15s\n", 
           "ID", "STATUS", "TITLE", "DEFER", "DUE", "FLG", "RECURRENCE");
    printf("--------------------------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        Task* t = &tasks[i];
        
        char defer_str[16];
        char due_str[16];
        format_date(t->defer_at, defer_str, sizeof(defer_str));
        format_date(t->due_at, due_str, sizeof(due_str));
        
        printf("%-4d %-10s %-40s %-12s %-12s %-3s %-15s\n",
               t->id,
               format_status(t->status),
               t->title,
               defer_str,
               due_str,
               t->flagged ? "YES" : "NO",
               format_recurrence(t->recurrence, t->recurrence_interval));
    }
    
    printf("\nTotal: %d task(s)\n", count);
    
    free(tasks);
    return 0;
}

// Command: add a task
static int cmd_add(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: samfocus-cli add <title> [--defer YYYY-MM-DD] [--due YYYY-MM-DD] [--flag]\n");
        return 1;
    }
    
    char title[256];
    strncpy(title, argv[1], sizeof(title) - 1);
    title[sizeof(title) - 1] = '\0';
    
    int task_id = db_insert_task(title, TASK_STATUS_INBOX);
    if (task_id < 0) {
        fprintf(stderr, "Error adding task: %s\n", db_get_error());
        return 1;
    }
    
    // Parse optional arguments
    time_t defer_at = 0;
    time_t due_at = 0;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--defer") == 0 && i + 1 < argc) {
            struct tm tm = {0};
            if (sscanf(argv[i + 1], "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
                tm.tm_year -= 1900;
                tm.tm_mon -= 1;
                defer_at = mktime(&tm);
                db_update_task_defer_at(task_id, defer_at);
            }
            i++;
        } else if (strcmp(argv[i], "--due") == 0 && i + 1 < argc) {
            struct tm tm = {0};
            if (sscanf(argv[i + 1], "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
                tm.tm_year -= 1900;
                tm.tm_mon -= 1;
                due_at = mktime(&tm);
                db_update_task_due_at(task_id, due_at);
            }
            i++;
        } else if (strcmp(argv[i], "--flag") == 0) {
            db_update_task_flagged(task_id, 1);
        }
    }
    
    printf("Task added successfully (ID: %d)\n", task_id);
    return 0;
}

// Command: complete a task
static int cmd_complete(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: samfocus-cli complete <task_id>\n");
        return 1;
    }
    
    int task_id = atoi(argv[1]);
    if (task_id <= 0) {
        fprintf(stderr, "Invalid task ID\n");
        return 1;
    }
    
    // Load the task to check if it's recurring
    Task* tasks = NULL;
    int count = 0;
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        fprintf(stderr, "Error loading tasks: %s\n", db_get_error());
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
        fprintf(stderr, "Task not found\n");
        free(tasks);
        return 1;
    }
    
    // Complete the task
    if (db_update_task_status(task_id, TASK_STATUS_DONE) != 0) {
        fprintf(stderr, "Error completing task: %s\n", db_get_error());
        free(tasks);
        return 1;
    }
    
    // If recurring, create next instance
    if (task->recurrence != RECUR_NONE) {
        int new_id = db_create_recurring_instance(task);
        if (new_id > 0) {
            printf("Task completed and next instance created (ID: %d)\n", new_id);
        } else {
            printf("Task completed (warning: could not create next instance)\n");
        }
    } else {
        printf("Task completed successfully\n");
    }
    
    free(tasks);
    return 0;
}

// Command: delete a task
static int cmd_delete(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: samfocus-cli delete <task_id>\n");
        return 1;
    }
    
    int task_id = atoi(argv[1]);
    if (task_id <= 0) {
        fprintf(stderr, "Invalid task ID\n");
        return 1;
    }
    
    if (db_delete_task(task_id) != 0) {
        fprintf(stderr, "Error deleting task: %s\n", db_get_error());
        return 1;
    }
    
    printf("Task deleted successfully\n");
    return 0;
}

// Command: show task details
static int cmd_show(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: samfocus-cli show <task_id>\n");
        return 1;
    }
    
    int task_id = atoi(argv[1]);
    if (task_id <= 0) {
        fprintf(stderr, "Invalid task ID\n");
        return 1;
    }
    
    Task* tasks = NULL;
    int count = 0;
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        fprintf(stderr, "Error loading tasks: %s\n", db_get_error());
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
        fprintf(stderr, "Task not found\n");
        free(tasks);
        return 1;
    }
    
    char defer_str[32];
    char due_str[32];
    char created_str[32];
    char modified_str[32];
    
    format_date(task->defer_at, defer_str, sizeof(defer_str));
    format_date(task->due_at, due_str, sizeof(due_str));
    format_date(task->created_at, created_str, sizeof(created_str));
    format_date(task->modified_at, modified_str, sizeof(modified_str));
    
    printf("\n");
    printf("Task ID:       %d\n", task->id);
    printf("Title:         %s\n", task->title);
    printf("Status:        %s\n", format_status(task->status));
    printf("Project ID:    %d\n", task->project_id);
    printf("Flagged:       %s\n", task->flagged ? "YES" : "NO");
    printf("Defer Date:    %s\n", defer_str);
    printf("Due Date:      %s\n", due_str);
    printf("Created:       %s\n", created_str);
    printf("Modified:      %s\n", modified_str);
    printf("Recurrence:    %s\n", format_recurrence(task->recurrence, task->recurrence_interval));
    printf("Order Index:   %d\n", task->order_index);
    
    if (task->notes[0] != '\0') {
        printf("\nNotes:\n%s\n", task->notes);
    }
    
    printf("\n");
    
    free(tasks);
    return 0;
}

// Command: list projects
static int cmd_projects(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    Project* projects = NULL;
    int count = 0;
    
    if (db_load_projects(&projects, &count) != 0) {
        fprintf(stderr, "Error loading projects: %s\n", db_get_error());
        return 1;
    }
    
    printf("%-4s %-12s %-40s\n", "ID", "TYPE", "TITLE");
    printf("----------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        Project* p = &projects[i];
        const char* type_str = (p->type == PROJECT_TYPE_SEQUENTIAL) ? "SEQUENTIAL" : "PARALLEL";
        printf("%-4d %-12s %-40s\n", p->id, type_str, p->title);
    }
    
    printf("\nTotal: %d project(s)\n", count);
    
    free(projects);
    return 0;
}

// Command: show today's tasks
static int cmd_today(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    Task* tasks = NULL;
    int count = 0;
    
    if (db_load_tasks(&tasks, &count, -1) != 0) {
        fprintf(stderr, "Error loading tasks: %s\n", db_get_error());
        return 1;
    }
    
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);
    
    printf("Tasks for today:\n");
    printf("%-4s %-40s %-12s %-3s\n", "ID", "TITLE", "DUE", "FLG");
    printf("----------------------------------------------------------------\n");
    
    int today_count = 0;
    for (int i = 0; i < count; i++) {
        Task* t = &tasks[i];
        
        if (t->status == TASK_STATUS_DONE) continue;
        
        // Show if defer_at is today or earlier (or no defer date)
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
            
            printf("%-4d %-40s %-12s %-3s\n",
                   t->id, t->title, due_str, t->flagged ? "YES" : "NO");
            today_count++;
        }
    }
    
    printf("\nTotal: %d task(s) available today\n", today_count);
    
    free(tasks);
    return 0;
}

// Show usage
static void print_usage(void) {
    printf("SamFocus CLI v%s - Command-line task management\n\n", VERSION);
    printf("Usage: samfocus-cli <command> [options]\n\n");
    printf("Commands:\n");
    printf("  list [--inbox|--active|--done]  List all tasks\n");
    printf("  add <title> [options]            Add a new task\n");
    printf("    Options: --defer YYYY-MM-DD, --due YYYY-MM-DD, --flag\n");
    printf("  complete <id>                    Mark task as complete\n");
    printf("  delete <id>                      Delete a task\n");
    printf("  show <id>                        Show task details\n");
    printf("  projects                         List all projects\n");
    printf("  today                            Show today's available tasks\n");
    printf("  help                             Show this help\n");
    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    char* db_path = get_db_path();
    if (!db_path) {
        return 1;
    }
    
    if (db_init(db_path) != 0) {
        fprintf(stderr, "Error: Could not initialize database: %s\n", db_get_error());
        return 1;
    }
    
    if (db_create_schema() != 0) {
        fprintf(stderr, "Error: Could not create schema: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    const char* cmd = argv[1];
    int result = 0;
    
    if (strcmp(cmd, "list") == 0) {
        result = cmd_list(argc - 1, argv + 1);
    } else if (strcmp(cmd, "add") == 0) {
        result = cmd_add(argc - 1, argv + 1);
    } else if (strcmp(cmd, "complete") == 0) {
        result = cmd_complete(argc - 1, argv + 1);
    } else if (strcmp(cmd, "delete") == 0) {
        result = cmd_delete(argc - 1, argv + 1);
    } else if (strcmp(cmd, "show") == 0) {
        result = cmd_show(argc - 1, argv + 1);
    } else if (strcmp(cmd, "projects") == 0) {
        result = cmd_projects(argc - 1, argv + 1);
    } else if (strcmp(cmd, "today") == 0) {
        result = cmd_today(argc - 1, argv + 1);
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0) {
        print_usage();
    } else {
        fprintf(stderr, "Unknown command: %s\n\n", cmd);
        print_usage();
        result = 1;
    }
    
    db_close();
    return result;
}
