#include "sidebar.h"
#include "../db/database.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#define INPUT_BUF_SIZE 256

static char new_project_buffer[INPUT_BUF_SIZE] = {0};
static int editing_project_id = -1;
static char edit_project_buffer[INPUT_BUF_SIZE] = {0};
static bool show_new_project_input = false;

static char new_context_buffer[INPUT_BUF_SIZE] = {0};
static int editing_context_id = -1;
static char edit_context_buffer[INPUT_BUF_SIZE] = {0};
static bool show_new_context_input = false;

void sidebar_init(void) {
    new_project_buffer[0] = '\0';
    editing_project_id = -1;
    edit_project_buffer[0] = '\0';
    show_new_project_input = false;
    
    new_context_buffer[0] = '\0';
    editing_context_id = -1;
    edit_context_buffer[0] = '\0';
    show_new_context_input = false;
}

// Helper function to count tasks for Today perspective
static int count_today_tasks(Task* tasks, int task_count) {
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        
        bool include = false;
        if (tasks[i].due_at > 0) {
            struct tm* due_tm = localtime(&tasks[i].due_at);
            if (due_tm->tm_year < now_tm->tm_year ||
                (due_tm->tm_year == now_tm->tm_year && due_tm->tm_yday <= now_tm->tm_yday)) {
                include = true;
            }
        } else {
            include = true;
        }
        if (include) count++;
    }
    return count;
}

// Helper function to count tasks for Anytime perspective
static int count_anytime_tasks(Task* tasks, int task_count) {
    time_t now = time(NULL);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        count++;
    }
    return count;
}

// Helper function to count flagged tasks
static int count_flagged_tasks(Task* tasks, int task_count) {
    time_t now = time(NULL);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        if (tasks[i].flagged) count++;
    }
    return count;
}

// Helper function to count inbox tasks
static int count_inbox_tasks(Task* tasks, int task_count) {
    time_t now = time(NULL);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        if (tasks[i].project_id == 0) count++;
    }
    return count;
}

// Helper function to count completed tasks
static int count_completed_tasks(Task* tasks, int task_count) {
    int count = 0;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) count++;
    }
    return count;
}

// Helper function to count tasks for a specific project
static int count_project_tasks(Task* tasks, int task_count, int project_id) {
    time_t now = time(NULL);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        if (tasks[i].project_id == project_id) count++;
    }
    return count;
}

// Helper function to count tasks for a specific context
static int count_context_tasks(Task* tasks, int task_count, int context_id, Context* contexts, int context_count) {
    time_t now = time(NULL);
    int count = 0;
    
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].status == TASK_STATUS_DONE) continue;
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) continue;
        
        // Check if this task has the context
        Context* task_contexts = NULL;
        int tc_count = 0;
        if (db_get_task_contexts(tasks[i].id, &task_contexts, &tc_count) == 0) {
            for (int j = 0; j < tc_count; j++) {
                if (task_contexts[j].id == context_id) {
                    count++;
                    break;
                }
            }
            if (task_contexts) free(task_contexts);
        }
    }
    return count;
}

void sidebar_render(Project* projects, int project_count, Context* contexts, int context_count,
                   Task* tasks, int task_count,
                   int* selected_project_id, int* selected_context_id, int* needs_reload) {
    *needs_reload = 0;
    
    // Sidebar window (fixed position, set by main.c)
    int window_flags = ImGuiWindowFlags_NoCollapse | 
                       ImGuiWindowFlags_NoMove | 
                       ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoTitleBar;
    igBegin("Projects", NULL, window_flags);
    
    // Header
    igText("Projects");
    igSameLine(0, 10);
    if (igSmallButton("+")) {
        show_new_project_input = true;
    }
    
    igSeparator();
    igSpacing();
    
    // Perspectives section
    igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "Perspectives");
    igSpacing();
    
    // Today perspective (ID = -1)
    int today_count = count_today_tasks(tasks, task_count);
    char today_label[64];
    snprintf(today_label, sizeof(today_label), "Today (%d)", today_count);
    bool today_selected = (*selected_project_id == -1);
    if (igSelectable_Bool(today_label, today_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = -1;
    }
    
    // Anytime perspective (ID = -3)
    int anytime_count = count_anytime_tasks(tasks, task_count);
    char anytime_label[64];
    snprintf(anytime_label, sizeof(anytime_label), "Anytime (%d)", anytime_count);
    bool anytime_selected = (*selected_project_id == -3);
    if (igSelectable_Bool(anytime_label, anytime_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = -3;
    }
    
    // Flagged perspective (ID = -4)
    int flagged_count = count_flagged_tasks(tasks, task_count);
    char flagged_label[64];
    snprintf(flagged_label, sizeof(flagged_label), "Flagged (%d)", flagged_count);
    bool flagged_selected = (*selected_project_id == -4);
    if (igSelectable_Bool(flagged_label, flagged_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = -4;
    }
    
    // Inbox (always visible, ID = 0)
    int inbox_count = count_inbox_tasks(tasks, task_count);
    char inbox_label[64];
    snprintf(inbox_label, sizeof(inbox_label), "Inbox (%d)", inbox_count);
    bool inbox_selected = (*selected_project_id == 0);
    if (igSelectable_Bool(inbox_label, inbox_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = 0;
    }
    
    // Completed perspective (ID = -2)
    int completed_count = count_completed_tasks(tasks, task_count);
    char completed_label[64];
    snprintf(completed_label, sizeof(completed_label), "Completed (%d)", completed_count);
    bool completed_selected = (*selected_project_id == -2);
    if (igSelectable_Bool(completed_label, completed_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = -2;
    }
    
    igSpacing();
    igSeparator();
    igSpacing();
    
    // Projects section
    igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "Projects");
    igSpacing();
    
    // New project input
    if (show_new_project_input) {
        igSetKeyboardFocusHere(0);
        igPushItemWidth(-1);
        if (igInputText("##newproject", new_project_buffer, INPUT_BUF_SIZE, 
                       ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
            if (new_project_buffer[0] != '\0') {
                int project_id = db_insert_project(new_project_buffer, PROJECT_TYPE_SEQUENTIAL);
                if (project_id >= 0) {
                    new_project_buffer[0] = '\0';
                    show_new_project_input = false;
                    *needs_reload = 1;
                    *selected_project_id = project_id;
                } else {
                    printf("Failed to create project: %s\n", db_get_error());
                }
            }
        }
        igPopItemWidth();
        
        if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
            show_new_project_input = false;
            new_project_buffer[0] = '\0';
        }
        
        igSpacing();
    }
    
    // Project list
    for (int i = 0; i < project_count; i++) {
        Project* project = &projects[i];
        
        igPushID_Int(project->id);
        
        // Check if this project is selected
        bool is_selected = (*selected_project_id == project->id);
        
        // Project name (editable if editing)
        if (editing_project_id == project->id) {
            igSetKeyboardFocusHere(0);
            igPushItemWidth(-1);
            if (igInputText("##edit", edit_project_buffer, INPUT_BUF_SIZE,
                           ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
                if (edit_project_buffer[0] != '\0') {
                    if (db_update_project_title(project->id, edit_project_buffer) == 0) {
                        *needs_reload = 1;
                        editing_project_id = -1;
                    }
                }
            }
            igPopItemWidth();
            
            if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
                editing_project_id = -1;
            }
        } else {
            // Display mode
            int proj_count = count_project_tasks(tasks, task_count, project->id);
            char label[300];
            snprintf(label, sizeof(label), "%s %s (%d)", 
                    project->type == PROJECT_TYPE_SEQUENTIAL ? "→" : "⋯",
                    project->title, proj_count);
            
            if (igSelectable_Bool(label, is_selected, 0, (ImVec2){0, 0})) {
                *selected_project_id = project->id;
            }
            
            // Right-click context menu
            if (igBeginPopupContextItem(NULL, ImGuiPopupFlags_MouseButtonRight)) {
                if (igMenuItem_Bool("Rename", NULL, false, true)) {
                    editing_project_id = project->id;
                    strncpy(edit_project_buffer, project->title, INPUT_BUF_SIZE - 1);
                    edit_project_buffer[INPUT_BUF_SIZE - 1] = '\0';
                    igCloseCurrentPopup();
                }
                
                // Toggle type (Sequential <-> Parallel)
                const char* type_label = project->type == PROJECT_TYPE_SEQUENTIAL ? 
                    "Make Parallel" : "Make Sequential";
                if (igMenuItem_Bool(type_label, NULL, false, true)) {
                    ProjectType new_type = project->type == PROJECT_TYPE_SEQUENTIAL ? 
                        PROJECT_TYPE_PARALLEL : PROJECT_TYPE_SEQUENTIAL;
                    if (db_update_project_type(project->id, new_type) == 0) {
                        *needs_reload = 1;
                    }
                    igCloseCurrentPopup();
                }
                
                igSeparator();
                
                if (igMenuItem_Bool("Delete", NULL, false, true)) {
                    if (db_delete_project(project->id) == 0) {
                        *needs_reload = 1;
                        if (*selected_project_id == project->id) {
                            *selected_project_id = 0;
                        }
                    }
                    igCloseCurrentPopup();
                }
                
                igEndPopup();
            }
        }
        
        igPopID();
    }
    
    igSpacing();
    igSeparator();
    igSpacing();
    
    // Contexts section
    igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, "Contexts");
    igSameLine(0, 10);
    if (igSmallButton("+##context")) {
        show_new_context_input = true;
    }
    igSpacing();
    
    // New context input
    if (show_new_context_input) {
        igSetKeyboardFocusHere(0);
        igPushItemWidth(-1);
        if (igInputText("##newcontext", new_context_buffer, INPUT_BUF_SIZE, 
                       ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
            if (new_context_buffer[0] != '\0') {
                // Use default gray color for now
                int context_id = db_insert_context(new_context_buffer, "#888888");
                if (context_id >= 0) {
                    new_context_buffer[0] = '\0';
                    show_new_context_input = false;
                    *needs_reload = 1;
                } else {
                    printf("Failed to create context: %s\n", db_get_error());
                }
            }
        }
        igPopItemWidth();
        
        if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
            show_new_context_input = false;
            new_context_buffer[0] = '\0';
        }
        
        igSpacing();
    }
    
    // Show "All" option to clear context filter
    bool no_context_filter = (*selected_context_id == 0);
    if (igSelectable_Bool("All", no_context_filter, 0, (ImVec2){0, 0})) {
        *selected_context_id = 0;
    }
    
    // Context list
    for (int i = 0; i < context_count; i++) {
        Context* context = &contexts[i];
        
        igPushID_Int(1000 + context->id);  // Offset to avoid ID collision with projects
        
        bool is_selected = (*selected_context_id == context->id);
        
        // Display context name with @ prefix and count
        int ctx_count = count_context_tasks(tasks, task_count, context->id, contexts, context_count);
        char label[80];
        snprintf(label, sizeof(label), "@%s (%d)", context->name, ctx_count);
        
        if (igSelectable_Bool(label, is_selected, 0, (ImVec2){0, 0})) {
            *selected_context_id = context->id;
        }
        
        // Right-click context menu
        if (igBeginPopupContextItem(NULL, ImGuiPopupFlags_MouseButtonRight)) {
            if (igMenuItem_Bool("Delete", NULL, false, true)) {
                if (db_delete_context(context->id) == 0) {
                    *needs_reload = 1;
                    if (*selected_context_id == context->id) {
                        *selected_context_id = 0;
                    }
                }
                igCloseCurrentPopup();
            }
            
            igEndPopup();
        }
        
        igPopID();
    }
    
    igEnd();
}

void sidebar_cleanup(void) {
    // Nothing to cleanup for now
}
