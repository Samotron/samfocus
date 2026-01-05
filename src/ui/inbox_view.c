#include "inbox_view.h"
#include "markdown.h"
#include "../db/database.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#define INPUT_BUF_SIZE 256
#define NOTES_BUF_SIZE 1024

// Parsed task data
typedef struct {
    char title[256];
    char context_names[5][64];  // Up to 5 contexts
    int context_count;
    bool flagged;
    time_t defer_at;
    time_t due_at;
} ParsedTask;

// Parse quick capture syntax: "Task title @context1 @context2 #tomorrow !flag"
static void parse_quick_capture(const char* input, ParsedTask* result) {
    memset(result, 0, sizeof(ParsedTask));
    
    char buffer[INPUT_BUF_SIZE];
    strncpy(buffer, input, INPUT_BUF_SIZE - 1);
    buffer[INPUT_BUF_SIZE - 1] = '\0';
    
    char title_parts[INPUT_BUF_SIZE] = {0};
    int title_len = 0;
    
    char* token = strtok(buffer, " ");
    while (token != NULL) {
        if (token[0] == '@' && strlen(token) > 1) {
            // Context
            if (result->context_count < 5) {
                strncpy(result->context_names[result->context_count], token + 1, 63);
                result->context_names[result->context_count][63] = '\0';
                result->context_count++;
            }
        } else if (token[0] == '#' && strlen(token) > 1) {
            // Date keyword
            const char* date_str = token + 1;
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            
            if (strcmp(date_str, "today") == 0) {
                result->defer_at = now;
            } else if (strcmp(date_str, "tomorrow") == 0) {
                tm_info->tm_mday += 1;
                result->defer_at = mktime(tm_info);
            } else if (strcmp(date_str, "weekend") == 0) {
                // Next Saturday
                int days_until_saturday = (6 - tm_info->tm_wday + 7) % 7;
                if (days_until_saturday == 0) days_until_saturday = 7;
                tm_info->tm_mday += days_until_saturday;
                result->defer_at = mktime(tm_info);
            }
        } else if (strcmp(token, "!flag") == 0 || strcmp(token, "!") == 0) {
            // Flag marker
            result->flagged = true;
        } else {
            // Regular title word
            if (title_len > 0) {
                title_parts[title_len++] = ' ';
            }
            int token_len = strlen(token);
            if (title_len + token_len < INPUT_BUF_SIZE - 1) {
                strcpy(title_parts + title_len, token);
                title_len += token_len;
            }
        }
        
        token = strtok(NULL, " ");
    }
    
    strncpy(result->title, title_parts, 255);
    result->title[255] = '\0';
}

static char input_buffer[INPUT_BUF_SIZE] = {0};
static int selected_task_index = -1;  // Index in the task list, not ID
static int editing_task_id = -1;
static char edit_buffer[INPUT_BUF_SIZE] = {0};
static bool focus_input = false;
static int editing_notes_task_id = -1;
static char notes_buffer[NOTES_BUF_SIZE] = {0};
static char search_buffer[INPUT_BUF_SIZE] = {0};
static bool notes_preview_mode = true;  // Start in preview mode

// Batch operations state
static bool batch_mode = false;
static bool selected_tasks[1000] = {0};  // Track selected task IDs
static int selected_count = 0;

// Dependency management state
static int editing_dependencies_task_id = -1;
static char dependency_input[INPUT_BUF_SIZE] = {0};

void inbox_view_init(void) {
    input_buffer[0] = '\0';
    selected_task_index = -1;
    editing_task_id = -1;
    edit_buffer[0] = '\0';
    focus_input = false;
}

void inbox_view_render(Task* tasks, int task_count, Project* projects, int project_count,
                       Context* contexts, int context_count, int selected_project_id, int* needs_reload) {
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
    if (selected_project_id == -6) {
        snprintf(window_title, sizeof(window_title), "Review - Stale Tasks");
    } else if (selected_project_id == -4) {
        snprintf(window_title, sizeof(window_title), "Flagged");
    } else if (selected_project_id == -3) {
        snprintf(window_title, sizeof(window_title), "Anytime");
    } else if (selected_project_id == -2) {
        snprintf(window_title, sizeof(window_title), "Completed");
    } else if (selected_project_id == -1) {
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
    igTextDisabled("Quick syntax: @context #tomorrow !flag");
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
            // Parse quick capture syntax
            ParsedTask parsed;
            parse_quick_capture(input_buffer, &parsed);
            
            // Use original input as title if parsing didn't extract anything
            const char* task_title = parsed.title[0] != '\0' ? parsed.title : input_buffer;
            
            int task_id = db_insert_task(task_title, TASK_STATUS_INBOX);
            if (task_id >= 0) {
                // Apply parsed attributes
                if (parsed.flagged) {
                    db_update_task_flagged(task_id, 1);
                }
                if (parsed.defer_at > 0) {
                    db_update_task_defer_at(task_id, parsed.defer_at);
                }
                
                // Add contexts
                for (int i = 0; i < parsed.context_count; i++) {
                    // Find or create context
                    int context_id = -1;
                    for (int j = 0; j < context_count; j++) {
                        if (strcmp(contexts[j].name, parsed.context_names[i]) == 0) {
                            context_id = contexts[j].id;
                            break;
                        }
                    }
                    
                    // Create context if it doesn't exist
                    if (context_id < 0) {
                        context_id = db_insert_context(parsed.context_names[i], "#888888");
                    }
                    
                    // Add context to task
                    if (context_id >= 0) {
                        db_add_context_to_task(task_id, context_id);
                    }
                }
                
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
    
    // Task list header with search
    igText("Tasks (%d):", task_count);
    igSameLine(0, 10);
    igPushItemWidth(200);
    igInputTextWithHint("##search", "Search...", search_buffer, INPUT_BUF_SIZE, 
                        ImGuiInputTextFlags_None, NULL, NULL);
    igPopItemWidth();
    
    // Batch mode controls
    igSameLine(0, 20);
    if (igButton(batch_mode ? "Exit Batch Mode" : "Batch Select", (ImVec2){0, 0})) {
        batch_mode = !batch_mode;
        if (!batch_mode) {
            // Clear selections when exiting
            memset(selected_tasks, 0, sizeof(selected_tasks));
            selected_count = 0;
        }
    }
    
    if (batch_mode && selected_count > 0) {
        igSameLine(0, 10);
        igText("(%d selected)", selected_count);
        igSameLine(0, 10);
        
        // Batch actions
        if (igButton("Complete All", (ImVec2){0, 0})) {
            for (int i = 0; i < task_count; i++) {
                if (selected_tasks[tasks[i].id]) {
                    db_update_task_status(tasks[i].id, TASK_STATUS_DONE);
                    if (tasks[i].recurrence != RECUR_NONE) {
                        db_create_recurring_instance(&tasks[i]);
                    }
                }
            }
            *needs_reload = 1;
            memset(selected_tasks, 0, sizeof(selected_tasks));
            selected_count = 0;
        }
        
        igSameLine(0, 5);
        if (igButton("Delete All", (ImVec2){0, 0})) {
            for (int i = 0; i < task_count; i++) {
                if (selected_tasks[tasks[i].id]) {
                    db_delete_task(tasks[i].id);
                }
            }
            *needs_reload = 1;
            memset(selected_tasks, 0, sizeof(selected_tasks));
            selected_count = 0;
        }
        
        igSameLine(0, 5);
        if (igButton("Flag All", (ImVec2){0, 0})) {
            for (int i = 0; i < task_count; i++) {
                if (selected_tasks[tasks[i].id]) {
                    db_update_task_flagged(tasks[i].id, 1);
                }
            }
            *needs_reload = 1;
        }
    }
    
    igSpacing();
    
    if (task_count == 0) {
        igTextDisabled("No tasks yet. Add one above!");
        selected_task_index = -1;
    } else {
        // Keyboard navigation (when not focused on input)
        if (!input_has_focus && editing_task_id == -1) {
            // Ctrl+Up/Down for reordering
            if (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_UpArrow, false)) {
                if (selected_task_index > 0 && selected_task_index < task_count) {
                    Task* selected = &tasks[selected_task_index];
                    Task* above = &tasks[selected_task_index - 1];
                    
                    // Swap order_index values
                    int temp_order = selected->order_index;
                    if (db_update_task_order_index(selected->id, above->order_index) == 0 &&
                        db_update_task_order_index(above->id, temp_order) == 0) {
                        *needs_reload = 1;
                        selected_task_index--;
                    }
                }
            } else if (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_DownArrow, false)) {
                if (selected_task_index >= 0 && selected_task_index < task_count - 1) {
                    Task* selected = &tasks[selected_task_index];
                    Task* below = &tasks[selected_task_index + 1];
                    
                    // Swap order_index values
                    int temp_order = selected->order_index;
                    if (db_update_task_order_index(selected->id, below->order_index) == 0 &&
                        db_update_task_order_index(below->id, temp_order) == 0) {
                        *needs_reload = 1;
                        selected_task_index++;
                    }
                }
            }
            // Arrow keys for navigation (only when Ctrl is not held)
            else if (igIsKeyPressed_Bool(ImGuiKey_DownArrow, false)) {
                if (selected_task_index < 0) {
                    selected_task_index = 0;
                } else if (selected_task_index < task_count - 1) {
                    selected_task_index++;
                }
            }
            else if (igIsKeyPressed_Bool(ImGuiKey_UpArrow, false)) {
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
                        // If completing a recurring task, create next instance
                        if (new_status == TASK_STATUS_DONE && selected->recurrence != RECUR_NONE) {
                            db_create_recurring_instance(selected);
                        }
                        *needs_reload = 1;
                    }
                }
                
                // F key to toggle flag
                if (igIsKeyPressed_Bool(ImGuiKey_F, false)) {
                    int new_flagged = selected->flagged ? 0 : 1;
                    if (db_update_task_flagged(selected->id, new_flagged) == 0) {
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
            
            // Filter by search text
            if (search_buffer[0] != '\0') {
                // Simple case-insensitive substring search
                char lower_title[256];
                char lower_search[256];
                
                for (int j = 0; task->title[j] && j < 255; j++) {
                    lower_title[j] = tolower(task->title[j]);
                    lower_title[j + 1] = '\0';
                }
                for (int j = 0; search_buffer[j] && j < 255; j++) {
                    lower_search[j] = tolower(search_buffer[j]);
                    lower_search[j + 1] = '\0';
                }
                
                if (!strstr(lower_title, lower_search)) {
                    continue;  // Skip this task if it doesn't match search
                }
            }
            
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
            
            // Checkbox - for batch selection or completion
            bool is_done = (task->status == TASK_STATUS_DONE);
            
            if (batch_mode) {
                // In batch mode, show selection checkbox
                bool is_selected = selected_tasks[task->id];
                if (igCheckbox("##select", &is_selected)) {
                    selected_tasks[task->id] = is_selected;
                    selected_count += is_selected ? 1 : -1;
                }
            } else {
                // Normal mode - completion checkbox
                if (igCheckbox("##done", &is_done)) {
                    TaskStatus new_status = is_done ? TASK_STATUS_DONE : TASK_STATUS_INBOX;
                    if (db_update_task_status(task->id, new_status) == 0) {
                        // If completing a recurring task, create next instance
                        if (is_done && task->recurrence != RECUR_NONE) {
                            db_create_recurring_instance(task);
                        }
                        *needs_reload = 1;
                    }
                }
            }
            
            igSameLine(0, 5);
            
            // Star/flag button
            const char* star_label = task->flagged ? "★" : "☆";
            ImVec4 star_color = task->flagged ? 
                (ImVec4){1.0f, 0.8f, 0.0f, 1.0f} :  // Gold when flagged
                (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};   // Gray when not flagged
            
            igPushStyleColor_Vec4(ImGuiCol_Text, star_color);
            if (igSmallButton(star_label)) {
                if (db_update_task_flagged(task->id, !task->flagged) == 0) {
                    *needs_reload = 1;
                }
            }
            igPopStyleColor(1);
            
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
                
                // Check if task is blocked by dependencies
                int is_blocked = db_is_task_blocked(task->id);
                
                if (is_done) {
                    igTextDisabled("%s", task->title);
                } else {
                    igText("%s", task->title);
                }
                
                // Show blocked indicator if task has incomplete dependencies
                if (is_blocked && !is_done) {
                    igSameLine(0, 5);
                    igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 0.6f, 0.2f, 1.0f});
                    igText("⏳");
                    igPopStyleColor(1);
                    if (igIsItemHovered(0)) {
                        igSetTooltip("Waiting on dependencies");
                    }
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
            
            // Reorder buttons (only show if not editing and there are multiple tasks)
            if (editing_task_id != task->id && task_count > 1) {
                igSameLine(0, 5);
                
                // Move up button (decrease order_index - move towards top)
                if (i > 0) {
                    if (igSmallButton("↑")) {
                        // Swap order with previous task
                        Task* prev_task = &tasks[i - 1];
                        int temp_order = task->order_index;
                        if (db_update_task_order_index(task->id, prev_task->order_index) == 0 &&
                            db_update_task_order_index(prev_task->id, temp_order) == 0) {
                            *needs_reload = 1;
                            selected_task_index = i - 1;
                        }
                    }
                } else {
                    igTextDisabled("↑");
                }
                
                igSameLine(0, 2);
                
                // Move down button (increase order_index - move towards bottom)
                if (i < task_count - 1) {
                    if (igSmallButton("↓")) {
                        // Swap order with next task
                        Task* next_task = &tasks[i + 1];
                        int temp_order = task->order_index;
                        if (db_update_task_order_index(task->id, next_task->order_index) == 0 &&
                            db_update_task_order_index(next_task->id, temp_order) == 0) {
                            *needs_reload = 1;
                            selected_task_index = i + 1;
                        }
                    }
                } else {
                    igTextDisabled("↓");
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
            
            // Context tags display (only show if not editing)
            if (editing_task_id != task->id && context_count > 0) {
                // Load contexts for this task
                Context* task_contexts = NULL;
                int task_context_count = 0;
                if (db_get_task_contexts(task->id, &task_contexts, &task_context_count) == 0 && task_context_count > 0) {
                    igSameLine(0, 10);
                    for (int j = 0; j < task_context_count; j++) {
                        char tag[80];
                        snprintf(tag, sizeof(tag), "@%s", task_contexts[j].name);
                        igTextColored((ImVec4){0.5f, 0.8f, 0.5f, 1.0f}, "%s", tag);
                        if (j < task_context_count - 1) {
                            igSameLine(0, 5);
                        }
                    }
                    free(task_contexts);
                }
                
                // Context management button
                igSameLine(0, 5);
                char ctx_btn[32];
                snprintf(ctx_btn, sizeof(ctx_btn), "@##ctx_%d", task->id);
                if (igSmallButton(ctx_btn)) {
                    char popup_id[48];
                    snprintf(popup_id, sizeof(popup_id), "context_popup_%d", task->id);
                    igOpenPopup_Str(popup_id, 0);
                }
                
                // Context management popup
                char popup_id[48];
                snprintf(popup_id, sizeof(popup_id), "context_popup_%d", task->id);
                if (igBeginPopup(popup_id, 0)) {
                    igText("Manage Contexts");
                    igSeparator();
                    
                    // Load current contexts for task
                    Context* current_contexts = NULL;
                    int current_context_count = 0;
                    db_get_task_contexts(task->id, &current_contexts, &current_context_count);
                    
                    // Show all contexts with checkboxes
                    for (int j = 0; j < context_count; j++) {
                        bool has_context = false;
                        for (int k = 0; k < current_context_count; k++) {
                            if (current_contexts[k].id == contexts[j].id) {
                                has_context = true;
                                break;
                            }
                        }
                        
                        char label[80];
                        snprintf(label, sizeof(label), "@%s", contexts[j].name);
                        if (igCheckbox(label, &has_context)) {
                            if (has_context) {
                                // Add context
                                if (db_add_context_to_task(task->id, contexts[j].id) == 0) {
                                    *needs_reload = 1;
                                }
                            } else {
                                // Remove context
                                if (db_remove_context_from_task(task->id, contexts[j].id) == 0) {
                                    *needs_reload = 1;
                                }
                            }
                        }
                    }
                    
                    if (current_contexts != NULL) {
                        free(current_contexts);
                    }
                    
                    igEndPopup();
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
            
            // Recurrence indicator/button
            if (editing_task_id != task->id) {
                igSameLine(0, 10);
                
                // Show recurrence icon if task has recurrence
                if (task->recurrence != RECUR_NONE) {
                    const char* recur_names[] = {"", "Daily", "Weekly", "Monthly", "Yearly"};
                    char recur_label[64];
                    if (task->recurrence_interval == 1) {
                        snprintf(recur_label, sizeof(recur_label), "🔄%s##recur_%d", 
                                recur_names[task->recurrence], task->id);
                    } else {
                        snprintf(recur_label, sizeof(recur_label), "🔄Every %d %s##recur_%d", 
                                task->recurrence_interval, recur_names[task->recurrence], task->id);
                    }
                    
                    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.2f, 0.6f, 0.4f, 1.0f});
                    if (igSmallButton(recur_label)) {
                        char popup_id[48];
                        snprintf(popup_id, sizeof(popup_id), "recur_popup_%d", task->id);
                        igOpenPopup_Str(popup_id, 0);
                    }
                    igPopStyleColor(1);
                    
                    // Recurrence popup
                    char recur_popup_id[48];
                    snprintf(recur_popup_id, sizeof(recur_popup_id), "recur_popup_%d", task->id);
                    if (igBeginPopup(recur_popup_id, 0)) {
                        igText("Recurrence Pattern");
                        igSeparator();
                        
                        if (igSelectable_Bool("None (Remove)", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_NONE, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Daily", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_DAILY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Weekly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_WEEKLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Monthly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_MONTHLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Yearly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_YEARLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        igEndPopup();
                    }
                } else {
                    // No recurrence - show "Repeat" button
                    char recur_btn[32];
                    snprintf(recur_btn, sizeof(recur_btn), "Repeat##recur_%d", task->id);
                    if (igSmallButton(recur_btn)) {
                        char popup_id[48];
                        snprintf(popup_id, sizeof(popup_id), "recur_popup_%d", task->id);
                        igOpenPopup_Str(popup_id, 0);
                    }
                    
                    // Recurrence popup
                    char recur_popup_id[48];
                    snprintf(recur_popup_id, sizeof(recur_popup_id), "recur_popup_%d", task->id);
                    if (igBeginPopup(recur_popup_id, 0)) {
                        igText("Set Recurrence");
                        igSeparator();
                        
                        if (igSelectable_Bool("Daily", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_DAILY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Weekly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_WEEKLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Monthly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_MONTHLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        if (igSelectable_Bool("Yearly", false, 0, (ImVec2){0, 0})) {
                            if (db_update_task_recurrence(task->id, RECUR_YEARLY, 1) == 0) {
                                *needs_reload = 1;
                            }
                            igCloseCurrentPopup();
                        }
                        
                        igEndPopup();
                    }
                }
            }
            
            // Notes button/indicator
            if (editing_task_id != task->id) {
                igSameLine(0, 10);
                bool has_notes = (task->notes[0] != '\0');
                char notes_btn[32];
                snprintf(notes_btn, sizeof(notes_btn), "%s##notes_%d", 
                        has_notes ? "Notes*" : "Notes", task->id);
                
                if (has_notes) {
                    igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.2f, 0.4f, 0.6f, 1.0f});
                }
                
                if (igSmallButton(notes_btn)) {
                    editing_notes_task_id = task->id;
                    strncpy(notes_buffer, task->notes, NOTES_BUF_SIZE - 1);
                    notes_buffer[NOTES_BUF_SIZE - 1] = '\0';
                    char popup_id[48];
                    snprintf(popup_id, sizeof(popup_id), "notes_popup_%d", task->id);
                    igOpenPopup_Str(popup_id, 0);
                }
                
                if (has_notes) {
                    igPopStyleColor(1);
                }
                
                // Notes popup
                char notes_popup_id[48];
                snprintf(notes_popup_id, sizeof(notes_popup_id), "notes_popup_%d", task->id);
                if (igBeginPopup(notes_popup_id, 0)) {
                    igText("Notes for: %s", task->title);
                    igSeparator();
                    
                    // Toggle between preview and edit modes
                    igSpacing();
                    if (igButton(notes_preview_mode ? "Edit" : "Preview", (ImVec2){80, 0})) {
                        notes_preview_mode = !notes_preview_mode;
                    }
                    igSameLine(0, 10);
                    igTextDisabled(notes_preview_mode ? "(Markdown Preview)" : "(Edit Mode)");
                    igSpacing();
                    
                    if (notes_preview_mode) {
                        // Preview mode with markdown rendering
                        igBeginChild_Str("notes_preview", (ImVec2){400, 200}, true, 0);
                        markdown_render(notes_buffer);
                        igEndChild();
                    } else {
                        // Edit mode with text input
                        igPushItemWidth(400);
                        if (igInputTextMultiline("##notes_edit", notes_buffer, NOTES_BUF_SIZE, 
                                                 (ImVec2){400, 200}, 0, NULL, NULL)) {
                            // Text changed
                        }
                        igPopItemWidth();
                    }
                    
                    igSpacing();
                    if (igButton("Save", (ImVec2){0, 0})) {
                        if (db_update_task_notes(task->id, notes_buffer) == 0) {
                            *needs_reload = 1;
                        }
                        igCloseCurrentPopup();
                    }
                    igSameLine(0, 10);
                    if (igButton("Cancel", (ImVec2){0, 0})) {
                        igCloseCurrentPopup();
                    }
                    
                    igEndPopup();
                }
            }
            
            // Dependencies button/indicator
            if (editing_task_id != task->id) {
                igSameLine(0, 10);
                
                // Check if task has dependencies
                int* dep_ids = NULL;
                int dep_count = 0;
                db_get_task_dependencies(task->id, &dep_ids, &dep_count);
                
                char deps_btn[32];
                snprintf(deps_btn, sizeof(deps_btn), "%s##deps_%d", 
                        dep_count > 0 ? "Deps*" : "Deps", task->id);
                
                // Check if task is blocked
                int is_blocked = db_is_task_blocked(task->id);
                
                if (dep_count > 0 || is_blocked) {
                    ImVec4 btn_color = is_blocked ? 
                        (ImVec4){0.6f, 0.3f, 0.2f, 1.0f} :  // Red if blocked
                        (ImVec4){0.3f, 0.5f, 0.4f, 1.0f};   // Green if has deps but not blocked
                    igPushStyleColor_Vec4(ImGuiCol_Button, btn_color);
                }
                
                if (igSmallButton(deps_btn)) {
                    editing_dependencies_task_id = task->id;
                    dependency_input[0] = '\0';
                    char popup_id[48];
                    snprintf(popup_id, sizeof(popup_id), "deps_popup_%d", task->id);
                    igOpenPopup_Str(popup_id, 0);
                }
                
                if (dep_count > 0 || is_blocked) {
                    igPopStyleColor(1);
                }
                
                if (dep_ids) {
                    free(dep_ids);
                }
                
                // Dependencies popup
                char deps_popup_id[48];
                snprintf(deps_popup_id, sizeof(deps_popup_id), "deps_popup_%d", task->id);
                if (igBeginPopup(deps_popup_id, 0)) {
                    igText("Dependencies for: %s", task->title);
                    igSeparator();
                    igSpacing();
                    
                    // Load and display dependencies
                    int* dependency_ids = NULL;
                    int dependency_count = 0;
                    if (db_get_task_dependencies(task->id, &dependency_ids, &dependency_count) == 0) {
                        if (dependency_count > 0) {
                            igText("This task depends on:");
                            igSpacing();
                            
                            for (int d = 0; d < dependency_count; d++) {
                                int dep_id = dependency_ids[d];
                                
                                // Find the dependency task
                                Task* dep_task = NULL;
                                for (int t = 0; t < task_count; t++) {
                                    if (tasks[t].id == dep_id) {
                                        dep_task = &tasks[t];
                                        break;
                                    }
                                }
                                
                                if (dep_task) {
                                    // Show status icon
                                    const char* status_icon = (dep_task->status == TASK_STATUS_DONE) ? "✓" : "○";
                                    igText("  %s Task #%d: %s", status_icon, dep_id, dep_task->title);
                                    igSameLine(0, 10);
                                    
                                    char remove_btn[32];
                                    snprintf(remove_btn, sizeof(remove_btn), "Remove##%d", dep_id);
                                    if (igSmallButton(remove_btn)) {
                                        db_remove_dependency(task->id, dep_id);
                                        *needs_reload = 1;
                                    }
                                } else {
                                    igText("  Task #%d (not found)", dep_id);
                                }
                            }
                            igSpacing();
                            igSeparator();
                            igSpacing();
                        } else {
                            igTextDisabled("No dependencies set");
                            igSpacing();
                        }
                        
                        if (dependency_ids) {
                            free(dependency_ids);
                        }
                    }
                    
                    // Add new dependency
                    igText("Add dependency (task ID):");
                    igPushItemWidth(200);
                    if (igInputTextWithHint("##dep_input", "Task ID...", dependency_input, 
                                           INPUT_BUF_SIZE, ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
                        int dep_id = atoi(dependency_input);
                        if (dep_id > 0 && dep_id != task->id) {
                            if (db_add_dependency(task->id, dep_id) == 0) {
                                *needs_reload = 1;
                                dependency_input[0] = '\0';
                            }
                        }
                    }
                    igPopItemWidth();
                    
                    igSpacing();
                    if (igButton("Close", (ImVec2){0, 0})) {
                        igCloseCurrentPopup();
                    }
                    
                    igEndPopup();
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
