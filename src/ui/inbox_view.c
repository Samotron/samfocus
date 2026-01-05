#include "inbox_view.h"
#include "../db/database.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

#include <string.h>
#include <stdio.h>

#define INPUT_BUF_SIZE 256

static char input_buffer[INPUT_BUF_SIZE] = {0};
static int selected_task_id = -1;
static char edit_buffer[INPUT_BUF_SIZE] = {0};

void inbox_view_init(void) {
    input_buffer[0] = '\0';
    selected_task_id = -1;
    edit_buffer[0] = '\0';
}

void inbox_view_render(Task* tasks, int task_count, int* needs_reload) {
    *needs_reload = 0;
    
    // Main window
    igBegin("Inbox", NULL, 0);
    
    // Input field for new tasks
    igText("Add new task:");
    igSameLine(0, 10);
    
    // Focus input on Ctrl+N
    ImGuiIO* io = igGetIO();
    if (io->KeyCtrl && igIsKeyPressed(igGetKeyIndex(ImGuiKey_N), false)) {
        igSetKeyboardFocusHere(0);
    }
    
    igPushItemWidth(-1);
    if (igInputText("##newtask", input_buffer, INPUT_BUF_SIZE, 
                    ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
        // Enter was pressed
        if (input_buffer[0] != '\0') {
            int task_id = db_insert_task(input_buffer, TASK_STATUS_INBOX);
            if (task_id >= 0) {
                input_buffer[0] = '\0';
                *needs_reload = 1;
            } else {
                printf("Failed to add task: %s\n", db_get_error());
            }
        }
    }
    igPopItemWidth();
    
    igSeparator();
    
    // Task list
    igText("Tasks (%d):", task_count);
    igSpacing();
    
    if (task_count == 0) {
        igTextDisabled("No tasks yet. Add one above!");
    } else {
        // Begin child window for scrollable task list
        igBeginChild_Str("TaskList", (ImVec2){0, 0}, false, 0);
        
        for (int i = 0; i < task_count; i++) {
            Task* task = &tasks[i];
            
            igPushID_Int(task->id);
            
            // Checkbox for completion
            bool is_done = (task->status == TASK_STATUS_DONE);
            if (igCheckbox("##done", &is_done)) {
                TaskStatus new_status = is_done ? TASK_STATUS_DONE : TASK_STATUS_INBOX;
                if (db_update_task_status(task->id, new_status) == 0) {
                    *needs_reload = 1;
                }
            }
            
            igSameLine(0, 10);
            
            // Task title (editable if selected)
            if (selected_task_id == task->id) {
                igPushItemWidth(-100);
                if (igInputText("##edit", edit_buffer, INPUT_BUF_SIZE, 
                               ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
                    // Save on Enter
                    if (edit_buffer[0] != '\0') {
                        if (db_update_task_title(task->id, edit_buffer) == 0) {
                            *needs_reload = 1;
                            selected_task_id = -1;
                        }
                    }
                }
                igPopItemWidth();
                
                // Cancel on Escape
                if (igIsKeyPressed(igGetKeyIndex(ImGuiKey_Escape), false)) {
                    selected_task_id = -1;
                }
            } else {
                // Display mode
                if (is_done) {
                    igTextDisabled("%s", task->title);
                } else {
                    igText("%s", task->title);
                }
                
                // Click to edit
                if (igIsItemClicked(ImGuiMouseButton_Left)) {
                    selected_task_id = task->id;
                    strncpy(edit_buffer, task->title, INPUT_BUF_SIZE - 1);
                    edit_buffer[INPUT_BUF_SIZE - 1] = '\0';
                }
            }
            
            // Delete button
            igSameLine(0, 10);
            if (igButton("Delete", (ImVec2){0, 0})) {
                if (db_delete_task(task->id) == 0) {
                    *needs_reload = 1;
                    if (selected_task_id == task->id) {
                        selected_task_id = -1;
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
