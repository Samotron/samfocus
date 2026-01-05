#include "launcher.h"
#include "../db/database.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>

#define INPUT_BUF_SIZE 512
#define MAX_RESULTS 10

static bool launcher_visible = false;
static char input_buffer[INPUT_BUF_SIZE] = {0};
static bool focus_input = true;

// Search results
typedef enum {
    ACTION_ADD_TASK,
    ACTION_SEARCH_TASK,
    ACTION_QUICK_COMMAND,
    ACTION_OPEN_PROJECT,
    ACTION_FILTER_CONTEXT
} ActionType;

typedef struct {
    ActionType type;
    char label[256];
    char description[128];
    int id;  // Task/Project/Context ID if applicable
} LauncherAction;

static LauncherAction results[MAX_RESULTS];
static int result_count = 0;
static int selected_index = 0;

void launcher_init(void) {
    launcher_visible = false;
    input_buffer[0] = '\0';
    focus_input = true;
}

void launcher_show(void) {
    launcher_visible = true;
    input_buffer[0] = '\0';
    focus_input = true;
    result_count = 0;
    selected_index = 0;
}

void launcher_hide(void) {
    launcher_visible = false;
    input_buffer[0] = '\0';
}

int launcher_is_visible(void) {
    return launcher_visible;
}

// Natural language parsing helpers
static int parse_date_keyword(const char* text, time_t* out_date) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    if (strstr(text, "today")) {
        *out_date = now;
        return 1;
    } else if (strstr(text, "tomorrow")) {
        *out_date = now + 86400;
        return 1;
    } else if (strstr(text, "next week")) {
        *out_date = now + (7 * 86400);
        return 1;
    } else if (strstr(text, "monday") || strstr(text, "mon")) {
        int days_until_monday = (8 - tm_info->tm_wday) % 7;
        if (days_until_monday == 0) days_until_monday = 7;
        *out_date = now + (days_until_monday * 86400);
        return 1;
    } else if (strstr(text, "friday") || strstr(text, "fri")) {
        int days_until_friday = (5 - tm_info->tm_wday + 7) % 7;
        if (days_until_friday == 0) days_until_friday = 7;
        *out_date = now + (days_until_friday * 86400);
        return 1;
    }
    
    return 0;
}

static void extract_task_components(const char* input, char* task_title, 
                                    time_t* due_date, time_t* defer_date,
                                    int* flagged, int* project_id) {
    char lower_input[INPUT_BUF_SIZE];
    strncpy(lower_input, input, INPUT_BUF_SIZE - 1);
    lower_input[INPUT_BUF_SIZE - 1] = '\0';
    
    // Convert to lowercase for parsing
    for (char* p = lower_input; *p; p++) {
        *p = tolower(*p);
    }
    
    // Check for flag indicator (! or "important")
    if (strchr(input, '!') || strstr(lower_input, "important") || strstr(lower_input, "urgent")) {
        *flagged = 1;
    }
    
    // Parse due date keywords
    if (strstr(lower_input, "due ")) {
        parse_date_keyword(lower_input, due_date);
    }
    
    // Parse defer date keywords
    if (strstr(lower_input, "defer ") || strstr(lower_input, "start ")) {
        parse_date_keyword(lower_input, defer_date);
    }
    
    // Extract clean task title (remove special keywords)
    strncpy(task_title, input, INPUT_BUF_SIZE - 1);
    task_title[INPUT_BUF_SIZE - 1] = '\0';
    
    // Remove "!" if present
    char* excl = strchr(task_title, '!');
    if (excl) *excl = ' ';
    
    // Remove common keywords
    char* keywords[] = {"due today", "due tomorrow", "due next week", 
                       "defer today", "defer tomorrow", "start tomorrow",
                       "important", "urgent", NULL};
    
    for (int i = 0; keywords[i] != NULL; i++) {
        char* pos = strstr(task_title, keywords[i]);
        if (pos) {
            // Replace with spaces
            for (size_t j = 0; j < strlen(keywords[i]); j++) {
                pos[j] = ' ';
            }
        }
    }
    
    // Trim leading/trailing whitespace
    char* start = task_title;
    while (*start && isspace(*start)) start++;
    
    if (start != task_title) {
        memmove(task_title, start, strlen(start) + 1);
    }
    
    // Trim trailing whitespace
    char* end = task_title + strlen(task_title) - 1;
    while (end > task_title && isspace(*end)) {
        *end = '\0';
        end--;
    }
}

static void fuzzy_search_tasks(const char* query, Task* tasks, int task_count) {
    result_count = 0;
    
    if (query[0] == '\0') return;
    
    char lower_query[INPUT_BUF_SIZE];
    strncpy(lower_query, query, INPUT_BUF_SIZE - 1);
    lower_query[INPUT_BUF_SIZE - 1] = '\0';
    for (char* p = lower_query; *p; p++) *p = tolower(*p);
    
    for (int i = 0; i < task_count && result_count < MAX_RESULTS; i++) {
        char lower_title[256];
        strncpy(lower_title, tasks[i].title, 255);
        lower_title[255] = '\0';
        for (char* p = lower_title; *p; p++) *p = tolower(*p);
        
        if (strstr(lower_title, lower_query)) {
            results[result_count].type = ACTION_SEARCH_TASK;
            snprintf(results[result_count].label, sizeof(results[result_count].label),
                    "%s", tasks[i].title);
            snprintf(results[result_count].description, sizeof(results[result_count].description),
                    "Open task #%d", tasks[i].id);
            results[result_count].id = tasks[i].id;
            result_count++;
        }
    }
}

static void generate_actions(const char* input, Task* tasks, int task_count,
                            Project* projects, int project_count,
                            Context* contexts, int context_count) {
    result_count = 0;
    
    if (input[0] == '\0') {
        // Show default actions when input is empty
        results[result_count].type = ACTION_ADD_TASK;
        snprintf(results[result_count].label, sizeof(results[result_count].label), 
                "Quick Add Task");
        snprintf(results[result_count].description, sizeof(results[result_count].description),
                "Type to add a new task");
        result_count++;
        return;
    }
    
    // Check for command prefixes
    if (input[0] == '/') {
        // Command mode
        results[result_count].type = ACTION_QUICK_COMMAND;
        snprintf(results[result_count].label, sizeof(results[result_count].label),
                "Command: %s", input + 1);
        snprintf(results[result_count].description, sizeof(results[result_count].description),
                "Execute command");
        result_count++;
        return;
    }
    
    if (input[0] == '@') {
        // Context filter mode
        char query[INPUT_BUF_SIZE];
        strncpy(query, input + 1, INPUT_BUF_SIZE - 1);
        query[INPUT_BUF_SIZE - 1] = '\0';
        
        for (int i = 0; i < context_count && result_count < MAX_RESULTS; i++) {
            if (strstr(contexts[i].name, query)) {
                results[result_count].type = ACTION_FILTER_CONTEXT;
                snprintf(results[result_count].label, sizeof(results[result_count].label),
                        "Filter: %s", contexts[i].name);
                snprintf(results[result_count].description, sizeof(results[result_count].description),
                        "Show tasks with this context");
                results[result_count].id = contexts[i].id;
                result_count++;
            }
        }
        return;
    }
    
    if (input[0] == '#') {
        // Project filter mode
        char query[INPUT_BUF_SIZE];
        strncpy(query, input + 1, INPUT_BUF_SIZE - 1);
        query[INPUT_BUF_SIZE - 1] = '\0';
        
        for (int i = 0; i < project_count && result_count < MAX_RESULTS; i++) {
            if (strstr(projects[i].title, query)) {
                results[result_count].type = ACTION_OPEN_PROJECT;
                snprintf(results[result_count].label, sizeof(results[result_count].label),
                        "Project: %s", projects[i].title);
                snprintf(results[result_count].description, sizeof(results[result_count].description),
                        "Show tasks in this project");
                results[result_count].id = projects[i].id;
                result_count++;
            }
        }
        return;
    }
    
    // Default: Add task action (always first)
    results[result_count].type = ACTION_ADD_TASK;
    snprintf(results[result_count].label, sizeof(results[result_count].label),
            "Add: %s", input);
    snprintf(results[result_count].description, sizeof(results[result_count].description),
            "Create new task");
    result_count++;
    
    // Add search results
    fuzzy_search_tasks(input, tasks, task_count);
}

void launcher_render(Task* tasks, int task_count,
                     Project* projects, int project_count,
                     Context* contexts, int context_count,
                     int* needs_reload) {
    if (!launcher_visible) return;
    
    // Center the launcher window
    ImGuiIO* io = igGetIO_Nil();
    ImVec2 center = {io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.3f};
    igSetNextWindowPos(center, ImGuiCond_Always, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){600, 400}, ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings;
    
    if (igBegin("##Launcher", NULL, flags)) {
        // Title
        igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.4f, 0.7f, 1.0f, 1.0f});
        igText("⚡ Quick Launcher");
        igPopStyleColor(1);
        igSameLine(0, 0);
        igSpacing();
        igSameLine(0, 0);
        igTextDisabled("(Ctrl+Space to close)");
        
        igSeparator();
        igSpacing();
        
        // Input field
        igPushItemWidth(-1);
        if (focus_input) {
            igSetKeyboardFocusHere(0);
            focus_input = false;
        }
        
        if (igInputTextWithHint("##launcher_input", 
                               "Type to add task, @ for context, # for project, / for command...",
                               input_buffer, INPUT_BUF_SIZE, 
                               ImGuiInputTextFlags_None, NULL, NULL)) {
            // Input changed, regenerate actions
            generate_actions(input_buffer, tasks, task_count, 
                           projects, project_count, contexts, context_count);
            selected_index = 0;
        }
        igPopItemWidth();
        
        // Handle keyboard navigation
        if (igIsKeyPressed_Bool(ImGuiKey_DownArrow, false)) {
            selected_index = (selected_index + 1) % (result_count > 0 ? result_count : 1);
        }
        if (igIsKeyPressed_Bool(ImGuiKey_UpArrow, false)) {
            selected_index = (selected_index - 1 + result_count) % (result_count > 0 ? result_count : 1);
        }
        
        // Handle Enter key to execute selected action
        if (igIsKeyPressed_Bool(ImGuiKey_Enter, false) && result_count > 0) {
            LauncherAction* action = &results[selected_index];
            
            if (action->type == ACTION_ADD_TASK) {
                // Parse input and create task
                char task_title[INPUT_BUF_SIZE] = {0};
                time_t due_date = 0;
                time_t defer_date = 0;
                int flagged = 0;
                int project_id = 0;
                
                extract_task_components(input_buffer, task_title, 
                                      &due_date, &defer_date, &flagged, &project_id);
                
                if (task_title[0] != '\0') {
                    int task_id = db_insert_task(task_title, TASK_STATUS_INBOX);
                    if (task_id > 0) {
                        if (due_date > 0) db_update_task_due_at(task_id, due_date);
                        if (defer_date > 0) db_update_task_defer_at(task_id, defer_date);
                        if (flagged) db_update_task_flagged(task_id, 1);
                        *needs_reload = 1;
                    }
                }
            }
            
            launcher_hide();
        }
        
        // Handle Escape to close
        if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
            launcher_hide();
        }
        
        igSpacing();
        igSeparator();
        igSpacing();
        
        // Show results
        if (result_count == 0 && input_buffer[0] != '\0') {
            igTextDisabled("No results found");
        } else {
            igBeginChild_Str("##results", (ImVec2){0, 0}, false, 0);
            
            for (int i = 0; i < result_count; i++) {
                bool is_selected = (i == selected_index);
                
                if (is_selected) {
                    ImVec4 selected_color = {0.3f, 0.5f, 0.8f, 0.4f};
                    ImVec2 p_min = igGetCursorScreenPos();
                    ImVec2 avail = igGetContentRegionAvail();
                    ImVec2 p_max = {p_min.x + avail.x, p_min.y + 60};
                    
                    ImDrawList* draw_list = igGetWindowDrawList();
                    ImDrawList_AddRectFilled(draw_list, p_min, p_max,
                                           igGetColorU32_Vec4(selected_color), 4.0f, 0);
                }
                
                // Icon based on action type
                const char* icon = "○";
                if (results[i].type == ACTION_ADD_TASK) icon = "➕";
                else if (results[i].type == ACTION_SEARCH_TASK) icon = "🔍";
                else if (results[i].type == ACTION_OPEN_PROJECT) icon = "📁";
                else if (results[i].type == ACTION_FILTER_CONTEXT) icon = "🏷️";
                else if (results[i].type == ACTION_QUICK_COMMAND) icon = "⚡";
                
                igText("%s", icon);
                igSameLine(0, 10);
                
                igBeginGroup();
                igText("%s", results[i].label);
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.6f, 0.6f, 0.6f, 1.0f});
                igText("%s", results[i].description);
                igPopStyleColor(1);
                igEndGroup();
                
                if (igIsItemClicked(ImGuiMouseButton_Left)) {
                    selected_index = i;
                    // Trigger Enter key logic here if needed
                }
                
                igSpacing();
            }
            
            igEndChild();
        }
        
        // Show helpful hints at bottom
        igSetCursorPosY(igGetWindowHeight() - 25);
        igSeparator();
        igTextDisabled("Tips: @ for contexts, # for projects, / for commands, ! for important");
    }
    igEnd();
    
    // Regenerate actions when launcher is first shown
    if (result_count == 0) {
        generate_actions(input_buffer, tasks, task_count,
                        projects, project_count, contexts, context_count);
    }
}
