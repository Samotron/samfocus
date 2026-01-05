#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

static char error_msg[512] = {0};

static void set_error(const char* msg) {
    snprintf(error_msg, sizeof(error_msg), "%s", msg);
}

const char* export_get_error(void) {
    return error_msg;
}

const char* export_get_default_dir(void) {
    static char export_dir[512];
    const char* home = getenv("HOME");
    if (!home) {
        return "/tmp";
    }
    
    snprintf(export_dir, sizeof(export_dir), "%s/.local/share/samfocus/exports", home);
    mkdir(export_dir, 0755);
    
    return export_dir;
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

// Helper to get project name
static const char* get_project_name(int project_id, Project* projects, int project_count) {
    if (project_id == 0 || projects == NULL) {
        return "None";
    }
    
    for (int i = 0; i < project_count; i++) {
        if (projects[i].id == project_id) {
            return projects[i].title;
        }
    }
    
    return "Unknown";
}

// Export as plain text
static int export_text(FILE* fp, Task* tasks, int task_count, Project* projects, int project_count) {
    fprintf(fp, "SamFocus Task Export - Text Format\n");
    fprintf(fp, "===================================\n");
    
    time_t now = time(NULL);
    char date_str[32];
    format_date(now, date_str, sizeof(date_str));
    fprintf(fp, "Exported: %s\n\n", date_str);
    
    // Group by status
    const char* status_names[] = {"INBOX", "ACTIVE", "DONE"};
    
    for (int status = 0; status <= 2; status++) {
        int count = 0;
        for (int i = 0; i < task_count; i++) {
            if ((int)tasks[i].status == status) count++;
        }
        
        if (count == 0) continue;
        
        fprintf(fp, "\n%s Tasks (%d)\n", status_names[status], count);
        fprintf(fp, "-------------------\n\n");
        
        for (int i = 0; i < task_count; i++) {
            Task* t = &tasks[i];
            if ((int)t->status != status) continue;
            
            fprintf(fp, "• %s", t->title);
            if (t->flagged) fprintf(fp, " ★");
            fprintf(fp, "\n");
            
            char defer_str[16], due_str[16], created_str[16];
            format_date(t->defer_at, defer_str, sizeof(defer_str));
            format_date(t->due_at, due_str, sizeof(due_str));
            format_date(t->created_at, created_str, sizeof(created_str));
            
            fprintf(fp, "  ID: %d\n", t->id);
            fprintf(fp, "  Project: %s\n", get_project_name(t->project_id, projects, project_count));
            fprintf(fp, "  Defer: %s  Due: %s\n", defer_str, due_str);
            fprintf(fp, "  Created: %s\n", created_str);
            
            if (t->recurrence != RECUR_NONE) {
                const char* recur_names[] = {"", "Daily", "Weekly", "Monthly", "Yearly"};
                fprintf(fp, "  Recurrence: %s", recur_names[t->recurrence]);
                if (t->recurrence_interval > 1) {
                    fprintf(fp, " (every %d)", t->recurrence_interval);
                }
                fprintf(fp, "\n");
            }
            
            if (t->notes[0] != '\0') {
                fprintf(fp, "  Notes: %s\n", t->notes);
            }
            
            fprintf(fp, "\n");
        }
    }
    
    fprintf(fp, "\nTotal: %d task(s)\n", task_count);
    return 0;
}

// Export as Markdown
static int export_markdown(FILE* fp, Task* tasks, int task_count, Project* projects, int project_count) {
    fprintf(fp, "# SamFocus Task Export\n\n");
    
    time_t now = time(NULL);
    char date_str[32];
    format_date(now, date_str, sizeof(date_str));
    fprintf(fp, "**Exported:** %s\n\n", date_str);
    
    // Group by status
    const char* status_names[] = {"Inbox", "Active", "Done"};
    
    for (int status = 0; status <= 2; status++) {
        int count = 0;
        for (int i = 0; i < task_count; i++) {
            if ((int)tasks[i].status == status) count++;
        }
        
        if (count == 0) continue;
        
        fprintf(fp, "## %s Tasks (%d)\n\n", status_names[status], count);
        
        for (int i = 0; i < task_count; i++) {
            Task* t = &tasks[i];
            if ((int)t->status != status) continue;
            
            // Checkbox format for done/not done
            if (t->status == TASK_STATUS_DONE) {
                fprintf(fp, "- [x] **%s**", t->title);
            } else {
                fprintf(fp, "- [ ] **%s**", t->title);
            }
            
            if (t->flagged) fprintf(fp, " ⭐");
            fprintf(fp, "\n");
            
            char defer_str[16], due_str[16];
            format_date(t->defer_at, defer_str, sizeof(defer_str));
            format_date(t->due_at, due_str, sizeof(due_str));
            
            fprintf(fp, "  - **ID:** %d\n", t->id);
            fprintf(fp, "  - **Project:** %s\n", get_project_name(t->project_id, projects, project_count));
            if (t->defer_at > 0) fprintf(fp, "  - **Defer:** %s\n", defer_str);
            if (t->due_at > 0) fprintf(fp, "  - **Due:** %s\n", due_str);
            
            if (t->recurrence != RECUR_NONE) {
                const char* recur_names[] = {"", "Daily", "Weekly", "Monthly", "Yearly"};
                fprintf(fp, "  - **Recurrence:** %s", recur_names[t->recurrence]);
                if (t->recurrence_interval > 1) {
                    fprintf(fp, " (every %d)", t->recurrence_interval);
                }
                fprintf(fp, "\n");
            }
            
            if (t->notes[0] != '\0') {
                fprintf(fp, "  - **Notes:** %s\n", t->notes);
            }
            
            fprintf(fp, "\n");
        }
    }
    
    fprintf(fp, "---\n");
    fprintf(fp, "**Total:** %d task(s)\n", task_count);
    return 0;
}

// Export as CSV
static int export_csv(FILE* fp, Task* tasks, int task_count, Project* projects, int project_count) {
    // CSV Header
    fprintf(fp, "ID,Title,Status,Project,Flagged,Defer Date,Due Date,Created,Modified,Recurrence,Notes\n");
    
    for (int i = 0; i < task_count; i++) {
        Task* t = &tasks[i];
        
        char defer_str[16], due_str[16], created_str[16], modified_str[16];
        format_date(t->defer_at, defer_str, sizeof(defer_str));
        format_date(t->due_at, due_str, sizeof(due_str));
        format_date(t->created_at, created_str, sizeof(created_str));
        format_date(t->modified_at, modified_str, sizeof(modified_str));
        
        const char* status_str = "INBOX";
        if (t->status == TASK_STATUS_ACTIVE) status_str = "ACTIVE";
        else if (t->status == TASK_STATUS_DONE) status_str = "DONE";
        
        // Escape quotes in title and notes
        char title_escaped[512];
        char notes_escaped[2048];
        snprintf(title_escaped, sizeof(title_escaped), "%s", t->title);
        snprintf(notes_escaped, sizeof(notes_escaped), "%s", t->notes);
        
        // Replace quotes with double quotes for CSV
        for (int j = 0; title_escaped[j]; j++) {
            if (title_escaped[j] == '"') title_escaped[j] = '\'';
        }
        for (int j = 0; notes_escaped[j]; j++) {
            if (notes_escaped[j] == '"') notes_escaped[j] = '\'';
        }
        
        const char* recur_str = "-";
        if (t->recurrence != RECUR_NONE) {
            static char recur_buf[32];
            const char* recur_names[] = {"", "Daily", "Weekly", "Monthly", "Yearly"};
            if (t->recurrence_interval == 1) {
                snprintf(recur_buf, sizeof(recur_buf), "%s", recur_names[t->recurrence]);
            } else {
                snprintf(recur_buf, sizeof(recur_buf), "Every %d %s", t->recurrence_interval, recur_names[t->recurrence]);
            }
            recur_str = recur_buf;
        }
        
        fprintf(fp, "%d,\"%s\",%s,\"%s\",%s,%s,%s,%s,%s,\"%s\",\"%s\"\n",
                t->id,
                title_escaped,
                status_str,
                get_project_name(t->project_id, projects, project_count),
                t->flagged ? "YES" : "NO",
                defer_str,
                due_str,
                created_str,
                modified_str,
                recur_str,
                notes_escaped);
    }
    
    return 0;
}

int export_tasks(const char* filepath, ExportFormat format,
                 Task* tasks, int task_count,
                 Project* projects, int project_count) {
    if (!filepath || !tasks) {
        set_error("Invalid parameters");
        return -1;
    }
    
    FILE* fp = fopen(filepath, "w");
    if (!fp) {
        set_error("Could not open file for writing");
        return -1;
    }
    
    int result = 0;
    switch (format) {
        case EXPORT_FORMAT_TEXT:
            result = export_text(fp, tasks, task_count, projects, project_count);
            break;
        case EXPORT_FORMAT_MARKDOWN:
            result = export_markdown(fp, tasks, task_count, projects, project_count);
            break;
        case EXPORT_FORMAT_CSV:
            result = export_csv(fp, tasks, task_count, projects, project_count);
            break;
        default:
            set_error("Unknown export format");
            result = -1;
            break;
    }
    
    fclose(fp);
    return result;
}

int export_create_backup(const char* db_path) {
    if (!db_path) {
        set_error("Invalid database path");
        return -1;
    }
    
    // Check if source file exists
    if (access(db_path, F_OK) != 0) {
        set_error("Database file does not exist");
        return -1;
    }
    
    // Create backup filename with timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    
    char backup_path[512];
    snprintf(backup_path, sizeof(backup_path), "%s.%s.bak", db_path, timestamp);
    
    // Copy file
    FILE* src = fopen(db_path, "rb");
    if (!src) {
        set_error("Could not open database for reading");
        return -1;
    }
    
    FILE* dst = fopen(backup_path, "wb");
    if (!dst) {
        fclose(src);
        set_error("Could not create backup file");
        return -1;
    }
    
    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            fclose(src);
            fclose(dst);
            set_error("Error writing backup file");
            return -1;
        }
    }
    
    fclose(src);
    fclose(dst);
    
    return 0;
}
