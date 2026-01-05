#include "sidebar.h"
#include "../db/database.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define INPUT_BUF_SIZE 256

static char new_project_buffer[INPUT_BUF_SIZE] = {0};
static int editing_project_id = -1;
static char edit_project_buffer[INPUT_BUF_SIZE] = {0};
static bool show_new_project_input = false;

void sidebar_init(void) {
    new_project_buffer[0] = '\0';
    editing_project_id = -1;
    edit_project_buffer[0] = '\0';
    show_new_project_input = false;
}

void sidebar_render(Project* projects, int project_count, int* selected_project_id, int* needs_reload) {
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
    bool today_selected = (*selected_project_id == -1);
    if (igSelectable_Bool("Today", today_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = -1;
    }
    
    // Inbox (always visible, ID = 0)
    bool inbox_selected = (*selected_project_id == 0);
    if (igSelectable_Bool("Inbox", inbox_selected, 0, (ImVec2){0, 0})) {
        *selected_project_id = 0;
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
            char label[300];
            snprintf(label, sizeof(label), "%s %s", 
                    project->type == PROJECT_TYPE_SEQUENTIAL ? "→" : "⋯",
                    project->title);
            
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
    
    igEnd();
}

void sidebar_cleanup(void) {
    // Nothing to cleanup for now
}
