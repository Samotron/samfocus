#include "command_palette.h"
#include <string.h>
#include <ctype.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

// Simple fuzzy match function (case-insensitive substring match)
static bool fuzzy_match(const char* pattern, const char* text) {
    if (pattern[0] == '\0') return true;
    
    char lower_pattern[256];
    char lower_text[256];
    
    // Convert to lowercase
    for (int i = 0; pattern[i] && i < 255; i++) {
        lower_pattern[i] = tolower(pattern[i]);
        lower_pattern[i + 1] = '\0';
    }
    
    for (int i = 0; text[i] && i < 255; i++) {
        lower_text[i] = tolower(text[i]);
        lower_text[i + 1] = '\0';
    }
    
    return strstr(lower_text, lower_pattern) != NULL;
}

void command_palette_init(CommandPaletteState* state) {
    state->is_open = false;
    state->search_input[0] = '\0';
    state->selected_index = 0;
    state->result_count = 0;
}

void command_palette_open(CommandPaletteState* state) {
    state->is_open = true;
    state->search_input[0] = '\0';
    state->selected_index = 0;
    state->result_count = 0;
}

void command_palette_close(CommandPaletteState* state) {
    state->is_open = false;
    state->search_input[0] = '\0';
    state->selected_index = 0;
    state->result_count = 0;
}

static void populate_results(CommandPaletteState* state,
                            Task* tasks, int task_count,
                            Project* projects, int project_count,
                            Context* contexts, int context_count) {
    state->result_count = 0;
    
    // Add quick actions if search is empty or starts with '/'
    if (state->search_input[0] == '\0' || state->search_input[0] == '/') {
        const struct {
            CommandAction action;
            const char* text;
        } actions[] = {
            {CMD_ACTION_FLAG, "/flag - Toggle flag on selected task"},
            {CMD_ACTION_DEFER_TODAY, "/defer today - Defer selected task to today"},
            {CMD_ACTION_DEFER_TOMORROW, "/defer tomorrow - Defer to tomorrow"},
            {CMD_ACTION_DEFER_WEEKEND, "/defer weekend - Defer to next Saturday"},
            {CMD_ACTION_DUE_TODAY, "/due today - Set due date to today"},
            {CMD_ACTION_DUE_TOMORROW, "/due tomorrow - Set due date to tomorrow"},
            {CMD_ACTION_COMPLETE, "/complete - Mark selected task as complete"},
            {CMD_ACTION_DELETE, "/delete - Delete selected task"}
        };
        
        for (int i = 0; i < 8 && state->result_count < 50; i++) {
            if (fuzzy_match(state->search_input, actions[i].text)) {
                CommandResult* r = &state->results[state->result_count++];
                r->type = CMD_TYPE_ACTION;
                r->action = actions[i].action;
                r->id = 0;
                strncpy(r->display_text, actions[i].text, 255);
                r->display_text[255] = '\0';
            }
        }
    }
    
    // Add matching tasks
    for (int i = 0; i < task_count && state->result_count < 50; i++) {
        if (fuzzy_match(state->search_input, tasks[i].title)) {
            CommandResult* r = &state->results[state->result_count++];
            r->type = CMD_TYPE_TASK;
            r->id = tasks[i].id;
            r->action = CMD_ACTION_NONE;
            
            // Format: "Task: Title [Project]"
            char prefix[32] = "Task: ";
            if (tasks[i].status == TASK_STATUS_DONE) {
                strcpy(prefix, "Task (Done): ");
            }
            snprintf(r->display_text, 255, "%s%s", prefix, tasks[i].title);
            r->display_text[255] = '\0';
        }
    }
    
    // Add matching projects
    for (int i = 0; i < project_count && state->result_count < 50; i++) {
        if (fuzzy_match(state->search_input, projects[i].title)) {
            CommandResult* r = &state->results[state->result_count++];
            r->type = CMD_TYPE_PROJECT;
            r->id = projects[i].id;
            r->action = CMD_ACTION_NONE;
            
            const char* type_icon = (projects[i].type == PROJECT_TYPE_PARALLEL) ? "⋯" : "→";
            snprintf(r->display_text, 255, "Project %s: %s", type_icon, projects[i].title);
            r->display_text[255] = '\0';
        }
    }
    
    // Add matching contexts
    for (int i = 0; i < context_count && state->result_count < 50; i++) {
        if (fuzzy_match(state->search_input, contexts[i].name)) {
            CommandResult* r = &state->results[state->result_count++];
            r->type = CMD_TYPE_CONTEXT;
            r->id = contexts[i].id;
            r->action = CMD_ACTION_NONE;
            snprintf(r->display_text, 255, "Context: @%s", contexts[i].name);
            r->display_text[255] = '\0';
        }
    }
}

bool command_palette_show(CommandPaletteState* state,
                         Task* tasks, int task_count,
                         Project* projects, int project_count,
                         Context* contexts, int context_count,
                         int current_task_id) {
    if (!state->is_open) return false;
    
    bool action_taken = false;
    
    // Center the palette
    ImGuiIO* io = igGetIO_Nil();
    ImVec2 center = {io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.3f};
    igSetNextWindowPos(center, ImGuiCond_Always, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){600, 400}, ImGuiCond_Always);
    
    if (igBeginPopupModal("Command Palette", &state->is_open, 
                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        
        // Focus on input when opened
        igSetKeyboardFocusHere(0);
        
        // Search input
        bool input_changed = igInputTextWithHint("##search", "Type to search tasks, projects, contexts, or /actions...",
                                                 state->search_input, sizeof(state->search_input),
                                                 ImGuiInputTextFlags_None, NULL, NULL);
        
        if (input_changed) {
            populate_results(state, tasks, task_count, projects, project_count, contexts, context_count);
            state->selected_index = 0;
        }
        
        // Handle keyboard navigation
        if (igIsKeyPressed_Bool(ImGuiKey_DownArrow, false)) {
            state->selected_index = (state->selected_index + 1) % (state->result_count > 0 ? state->result_count : 1);
        }
        if (igIsKeyPressed_Bool(ImGuiKey_UpArrow, false)) {
            state->selected_index = (state->selected_index - 1 + state->result_count) % (state->result_count > 0 ? state->result_count : 1);
        }
        
        // Handle selection with Enter
        if (igIsKeyPressed_Bool(ImGuiKey_Enter, false) && state->result_count > 0) {
            action_taken = true;
            command_palette_close(state);
        }
        
        // Escape to close
        if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
            command_palette_close(state);
        }
        
        igSeparator();
        
        // Results list
        if (state->result_count == 0 && state->search_input[0] == '\0') {
            populate_results(state, tasks, task_count, projects, project_count, contexts, context_count);
        }
        
        if (igBeginChild_Str("results", (ImVec2){0, 0}, false, 0)) {
            if (state->result_count == 0) {
                igTextDisabled("No results found");
            } else {
                for (int i = 0; i < state->result_count; i++) {
                    CommandResult* r = &state->results[i];
                    
                    // Highlight selected item
                    if (i == state->selected_index) {
                        ImVec4 selected_color = {0.26f, 0.59f, 0.98f, 0.31f};
                        igPushStyleColor_Vec4(ImGuiCol_Header, selected_color);
                    }
                    
                    bool is_selected = (i == state->selected_index);
                    if (igSelectable_Bool(r->display_text, is_selected, 0, (ImVec2){0, 0})) {
                        state->selected_index = i;
                        action_taken = true;
                        command_palette_close(state);
                    }
                    
                    if (i == state->selected_index) {
                        igPopStyleColor(1);
                    }
                }
            }
            igEndChild();
        }
        
        igEndPopup();
    }
    
    return action_taken;
}
