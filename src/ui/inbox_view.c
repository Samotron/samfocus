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

void inbox_view_render(Task* tasks, int task_count, int* needs_reload) {
    *needs_reload = 0;
    
    ImGuiIO* io = igGetIO_Nil();
    
    // Global shortcuts (work anywhere in the window)
    // Ctrl+N: Focus new task input
    if (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_N, false)) {
        focus_input = true;
        selected_task_index = -1;
        editing_task_id = -1;
    }
    
    // Main window
    igBegin("Inbox", NULL, 0);
    
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
            
            igPopID();
        }
        
        igEndChild();
    }
    
    igEnd();
}

void inbox_view_cleanup(void) {
    // Nothing to cleanup for now
}
