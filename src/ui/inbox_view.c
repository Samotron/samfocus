#include "inbox_view.h"
#include "../db/database.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define INPUT_BUF_SIZE 256

static char input_buffer[INPUT_BUF_SIZE] = {0};
static int selected_task_index = -1;  // Index in the task list, not ID
static int editing_task_id = -1;
static char edit_buffer[INPUT_BUF_SIZE] = {0};
static bool focus_input = false;

void inbox_view_init(void) {
    input_buffer[0] = '\0';
    selected_task_index = -1;
    editing_task_id = -1;
    edit_buffer[0] = '\0';
    focus_input = false;
}

void inbox_view_render(Task* tasks, int task_count, Project* projects, int project_count,
                       int selected_project_id, int* needs_reload) {
    *needs_reload = 0;
    
    ImGuiIO* io = igGetIO_Nil();
    
    // Global shortcuts (work anywhere in the window)
    // Ctrl+N: Focus new task input
    if (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_N, false)) {
        focus_input = true;
        selected_task_index = -1;
        editing_task_id = -1;
    }
    
    // Main window (fixed position, set by main.c)
    char window_title[300];
    if (selected_project_id == -1) {
        snprintf(window_title, sizeof(window_title), "Today");
    } else if (selected_project_id == 0) {
        snprintf(window_title, sizeof(window_title), "Inbox");
    } else {
        // Find project name
        const char* project_name = "Unknown";
        for (int i = 0; i < project_count; i++) {
            if (projects[i].id == selected_project_id) {
                project_name = projects[i].title;
                break;
            }
        }
        snprintf(window_title, sizeof(window_title), "Project: %s", project_name);
    }
    
    int window_flags = ImGuiWindowFlags_NoCollapse | 
                       ImGuiWindowFlags_NoMove | 
                       ImGuiWindowFlags_NoResize |
                       ImGuiWindowFlags_NoTitleBar;
    igBegin(window_title, NULL, window_flags);
    
    // Input field for new tasks
    igText("Add new task (Ctrl+N):");
    igSameLine(0, 10);
    
    // Handle focus request
    if (focus_input) {
        igSetKeyboardFocusHere(0);
        focus_input = false;
    }
    
    igPushItemWidth(-1);
    bool input_active = igInputText("##newtask", input_buffer, INPUT_BUF_SIZE, 
                                     ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);
    bool input_has_focus = igIsItemActive();
    
    if (input_active) {
        // Enter was pressed
        if (input_buffer[0] != '\0') {
            int task_id = db_insert_task(input_buffer, TASK_STATUS_INBOX);
            if (task_id >= 0) {
                input_buffer[0] = '\0';
                *needs_reload = 1;
                // Select the first task after adding
                selected_task_index = 0;
            } else {
                printf("Failed to add task: %s\n", db_get_error());
            }
        }
    }
    igPopItemWidth();
    
    igSeparator();
    
    // Task list header
    igText("Tasks (%d):", task_count);
    igSpacing();
    
    if (task_count == 0) {
        igTextDisabled("No tasks yet. Add one above!");
        selected_task_index = -1;
    } else {
        // Keyboard navigation (when not focused on input)
        if (!input_has_focus && editing_task_id == -1) {
            // Arrow keys for navigation
            if (igIsKeyPressed_Bool(ImGuiKey_DownArrow, false)) {
                if (selected_task_index < 0) {
                    selected_task_index = 0;
                } else if (selected_task_index < task_count - 1) {
                    selected_task_index++;
                }
            }
            
            if (igIsKeyPressed_Bool(ImGuiKey_UpArrow, false)) {
                if (selected_task_index > 0) {
                    selected_task_index--;
                } else if (selected_task_index < 0 && task_count > 0) {
                    selected_task_index = task_count - 1;
                }
            }
            
            // Home/End keys
            if (igIsKeyPressed_Bool(ImGuiKey_Home, false)) {
                selected_task_index = 0;
            }
            
            if (igIsKeyPressed_Bool(ImGuiKey_End, false)) {
                selected_task_index = task_count - 1;
            }
            
            // Ensure selection is valid
            if (selected_task_index >= task_count) {
                selected_task_index = task_count - 1;
            }
            
            // Actions on selected task
            if (selected_task_index >= 0 && selected_task_index < task_count) {
                Task* selected = &tasks[selected_task_index];
                
                // Delete key to delete
                if (igIsKeyPressed_Bool(ImGuiKey_Delete, false)) {
                    if (db_delete_task(selected->id) == 0) {
                        *needs_reload = 1;
                        // Adjust selection after deletion
                        if (selected_task_index >= task_count - 1) {
                            selected_task_index = task_count - 2;
                        }
                    }
                }
                
                // Space or Ctrl+Enter to toggle completion
                if (igIsKeyPressed_Bool(ImGuiKey_Space, false) || 
                    (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_Enter, false))) {
                    TaskStatus new_status = (selected->status == TASK_STATUS_DONE) ? 
                                           TASK_STATUS_INBOX : TASK_STATUS_DONE;
                    if (db_update_task_status(selected->id, new_status) == 0) {
                        *needs_reload = 1;
                    }
                }
                
                // Enter to edit
                if (igIsKeyPressed_Bool(ImGuiKey_Enter, false) && !io->KeyCtrl) {
                    editing_task_id = selected->id;
                    strncpy(edit_buffer, selected->title, INPUT_BUF_SIZE - 1);
                    edit_buffer[INPUT_BUF_SIZE - 1] = '\0';
                }
            }
        }
        
        // Begin child window for scrollable task list
        igBeginChild_Str("TaskList", (ImVec2){0, 0}, false, 0);
        
        for (int i = 0; i < task_count; i++) {
            Task* task = &tasks[i];
            bool is_selected = (i == selected_task_index);
            
            igPushID_Int(task->id);
            
            // Highlight selected task
            if (is_selected && editing_task_id != task->id) {
                ImVec4 selected_color = {0.3f, 0.5f, 0.8f, 0.3f};
                ImVec2 p_min = igGetCursorScreenPos();
                ImVec2 avail = igGetContentRegionAvail();
                ImVec2 p_max = {p_min.x + avail.x, p_min.y + igGetFrameHeight()};
                
                ImDrawList* draw_list = igGetWindowDrawList();
                ImDrawList_AddRectFilled(draw_list, p_min, p_max, 
                                        igGetColorU32_Vec4(selected_color), 0.0f, 0);
            }
            
            // Checkbox for completion
            bool is_done = (task->status == TASK_STATUS_DONE);
            if (igCheckbox("##done", &is_done)) {
                TaskStatus new_status = is_done ? TASK_STATUS_DONE : TASK_STATUS_INBOX;
                if (db_update_task_status(task->id, new_status) == 0) {
                    *needs_reload = 1;
                }
            }
            
            igSameLine(0, 10);
            
            // Task title (editable if this task is being edited)
            if (editing_task_id == task->id) {
                igSetKeyboardFocusHere(0);
                igPushItemWidth(-100);
                if (igInputText("##edit", edit_buffer, INPUT_BUF_SIZE, 
                               ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
                    // Save on Enter
                    if (edit_buffer[0] != '\0') {
                        if (db_update_task_title(task->id, edit_buffer) == 0) {
                            *needs_reload = 1;
                            editing_task_id = -1;
                        }
                    }
                }
                igPopItemWidth();
                
                // Cancel on Escape
                if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
                    editing_task_id = -1;
                }
            } else {
                // Display mode
                if (is_done) {
                    igTextDisabled("%s", task->title);
                } else {
                    igText("%s", task->title);
                }
                
                // Click to select and edit
                if (igIsItemClicked(ImGuiMouseButton_Left)) {
                    selected_task_index = i;
                    editing_task_id = task->id;
                    strncpy(edit_buffer, task->title, INPUT_BUF_SIZE - 1);
                    edit_buffer[INPUT_BUF_SIZE - 1] = '\0';
                }
                
                // Right-click or single click to just select
                if (igIsItemHovered(0) && igIsMouseClicked_Bool(ImGuiMouseButton_Right, false)) {
                    selected_task_index = i;
                }
            }
            
            // Delete button
            igSameLine(0, 10);
            if (igButton("Delete", (ImVec2){0, 0})) {
                if (db_delete_task(task->id) == 0) {
                    *needs_reload = 1;
                    if (selected_task_index == i) {
                        selected_task_index = (i > 0) ? i - 1 : -1;
                    } else if (selected_task_index > i) {
                        selected_task_index--;
                    }
                    if (editing_task_id == task->id) {
                        editing_task_id = -1;
                    }
                }
            }
            
            // Project assignment (only show if not editing)
            if (editing_task_id != task->id && project_count > 0) {
                igSameLine(0, 10);
                igText("→");
                igSameLine(0, 5);
                
                // Show current project or "None"
                const char* current_project = "None";
                if (task->project_id > 0) {
                    for (int j = 0; j < project_count; j++) {
                        if (projects[j].id == task->project_id) {
                            current_project = projects[j].title;
                            break;
                        }
                    }
                }
                
                char combo_label[300];
                snprintf(combo_label, sizeof(combo_label), "%s##project_%d", current_project, task->id);
                
                if (igBeginCombo(combo_label, current_project, 0)) {
                    // "None" option (unassign)
                    bool is_selected = (task->project_id == 0);
                    if (igSelectable_Bool("None", is_selected, 0, (ImVec2){0, 0})) {
                        if (db_assign_task_to_project(task->id, 0) == 0) {
                            *needs_reload = 1;
                        }
                    }
                    
                    // All projects
                    for (int j = 0; j < project_count; j++) {
                        is_selected = (task->project_id == projects[j].id);
                        if (igSelectable_Bool(projects[j].title, is_selected, 0, (ImVec2){0, 0})) {
                            if (db_assign_task_to_project(task->id, projects[j].id) == 0) {
                                *needs_reload = 1;
                            }
                        }
                    }
                    
                    igEndCombo();
                }
            }
            
            // Date information (only show if not editing)
            if (editing_task_id != task->id) {
                // Defer date
                if (task->defer_at > 0) {
                    igSameLine(0, 10);
                    struct tm* tm_defer = localtime(&task->defer_at);
                    char defer_str[32];
                    strftime(defer_str, sizeof(defer_str), "Defer:%m/%d", tm_defer);
                    igTextColored((ImVec4){0.7f, 0.7f, 1.0f, 1.0f}, "%s", defer_str);
                    igSameLine(0, 5);
                    char clear_defer_btn[32];
                    snprintf(clear_defer_btn, sizeof(clear_defer_btn), "X##defer_%d", task->id);
                    if (igSmallButton(clear_defer_btn)) {
                        if (db_update_task_defer_at(task->id, 0) == 0) {
                            *needs_reload = 1;
                        }
                    }
                } else {
                    igSameLine(0, 10);
                    char defer_btn[32];
                    snprintf(defer_btn, sizeof(defer_btn), "Defer##defer_%d", task->id);
                    if (igSmallButton(defer_btn)) {
                        char popup_id[48];
                        snprintf(popup_id, sizeof(popup_id), "defer_popup_%d", task->id);
                        igOpenPopup_Str(popup_id, 0);
                    }
                    
                    // Defer date picker popup
                    char popup_id[48];
                    snprintf(popup_id, sizeof(popup_id), "defer_popup_%d", task->id);
                    if (igBeginPopup(popup_id, 0)) {
                        time_t now = time(NULL);
                        struct tm* now_tm = localtime(&now);
                        
                        // Today (end of day)
                        if (igSelectable_Bool("Today", false, 0, (ImVec2){0, 0})) {
                            struct tm eod = *now_tm;
                            eod.tm_hour = 23; eod.tm_min = 59; eod.tm_sec = 59;
                            time_t eod_time = mktime(&eod);
                            if (db_update_task_defer_at(task->id, eod_time) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        // Tomorrow
                        if (igSelectable_Bool("Tomorrow", false, 0, (ImVec2){0, 0})) {
                            time_t tomorrow = now + (24 * 60 * 60);
                            if (db_update_task_defer_at(task->id, tomorrow) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        // Next Week
                        if (igSelectable_Bool("Next Week", false, 0, (ImVec2){0, 0})) {
                            time_t next_week = now + (7 * 24 * 60 * 60);
                            if (db_update_task_defer_at(task->id, next_week) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        igEndPopup();
                    }
                }
                
                // Due date
                if (task->due_at > 0) {
                    igSameLine(0, 10);
                    struct tm* tm_due = localtime(&task->due_at);
                    char due_str[32];
                    strftime(due_str, sizeof(due_str), "Due:%m/%d", tm_due);
                    
                    // Color based on due status
                    time_t now = time(NULL);
                    struct tm* now_tm = localtime(&now);
                    struct tm* due_tm = localtime(&task->due_at);
                    
                    // Check if overdue (due date < today)
                    bool is_overdue = false;
                    bool is_due_today = false;
                    
                    if (due_tm->tm_year < now_tm->tm_year ||
                        (due_tm->tm_year == now_tm->tm_year && due_tm->tm_yday < now_tm->tm_yday)) {
                        is_overdue = true;
                    } else if (due_tm->tm_year == now_tm->tm_year && due_tm->tm_yday == now_tm->tm_yday) {
                        is_due_today = true;
                    }
                    
                    if (is_overdue) {
                        igTextColored((ImVec4){1.0f, 0.3f, 0.3f, 1.0f}, "%s", due_str);
                    } else if (is_due_today) {
                        igTextColored((ImVec4){1.0f, 0.9f, 0.2f, 1.0f}, "%s", due_str);
                    } else {
                        igTextColored((ImVec4){0.7f, 1.0f, 0.7f, 1.0f}, "%s", due_str);
                    }
                    
                    igSameLine(0, 5);
                    char clear_due_btn[32];
                    snprintf(clear_due_btn, sizeof(clear_due_btn), "X##due_%d", task->id);
                    if (igSmallButton(clear_due_btn)) {
                        if (db_update_task_due_at(task->id, 0) == 0) {
                            *needs_reload = 1;
                        }
                    }
                } else {
                    igSameLine(0, 10);
                    char due_btn[32];
                    snprintf(due_btn, sizeof(due_btn), "Due##due_%d", task->id);
                    if (igSmallButton(due_btn)) {
                        char popup_id[48];
                        snprintf(popup_id, sizeof(popup_id), "due_popup_%d", task->id);
                        igOpenPopup_Str(popup_id, 0);
                    }
                    
                    // Due date picker popup
                    char due_popup_id[48];
                    snprintf(due_popup_id, sizeof(due_popup_id), "due_popup_%d", task->id);
                    if (igBeginPopup(due_popup_id, 0)) {
                        time_t now = time(NULL);
                        struct tm* now_tm = localtime(&now);
                        
                        // Today (end of day)
                        if (igSelectable_Bool("Today", false, 0, (ImVec2){0, 0})) {
                            struct tm eod = *now_tm;
                            eod.tm_hour = 23; eod.tm_min = 59; eod.tm_sec = 59;
                            time_t eod_time = mktime(&eod);
                            if (db_update_task_due_at(task->id, eod_time) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        // Tomorrow
                        if (igSelectable_Bool("Tomorrow", false, 0, (ImVec2){0, 0})) {
                            time_t tomorrow = now + (24 * 60 * 60);
                            if (db_update_task_due_at(task->id, tomorrow) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        // This Weekend (Saturday)
                        if (igSelectable_Bool("This Weekend", false, 0, (ImVec2){0, 0})) {
                            // Calculate days until Saturday
                            int days_until_saturday = (6 - now_tm->tm_wday + 7) % 7;
                            if (days_until_saturday == 0) days_until_saturday = 7; // If today is Saturday, go to next Saturday
                            time_t saturday = now + (days_until_saturday * 24 * 60 * 60);
                            if (db_update_task_due_at(task->id, saturday) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        // Next Week
                        if (igSelectable_Bool("Next Week", false, 0, (ImVec2){0, 0})) {
                            time_t next_week = now + (7 * 24 * 60 * 60);
                            if (db_update_task_due_at(task->id, next_week) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        igEndPopup();
                    }
                }
            }
            
            igPopID();
        }
        
        igEndChild();
    }
    
    igEnd();
}

void inbox_view_cleanup(void) {
    // Nothing to cleanup for now
}
