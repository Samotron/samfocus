#ifndef COMMAND_PALETTE_H
#define COMMAND_PALETTE_H

#include <stdbool.h>
#include "../core/task.h"
#include "../core/project.h"
#include "../core/context.h"

// Command palette result types
typedef enum {
    CMD_TYPE_NONE = 0,
    CMD_TYPE_TASK,
    CMD_TYPE_PROJECT,
    CMD_TYPE_CONTEXT,
    CMD_TYPE_ACTION
} CommandType;

// Command palette action types
typedef enum {
    CMD_ACTION_NONE = 0,
    CMD_ACTION_FLAG,
    CMD_ACTION_DEFER_TODAY,
    CMD_ACTION_DEFER_TOMORROW,
    CMD_ACTION_DEFER_WEEKEND,
    CMD_ACTION_DUE_TODAY,
    CMD_ACTION_DUE_TOMORROW,
    CMD_ACTION_COMPLETE,
    CMD_ACTION_DELETE
} CommandAction;

// Command palette result
typedef struct {
    CommandType type;
    int id;                 // Task/Project/Context ID
    CommandAction action;   // If type is ACTION
    char display_text[256]; // Display text for the item
} CommandResult;

// Command palette state
typedef struct {
    bool is_open;
    char search_input[256];
    int selected_index;
    CommandResult results[50];
    int result_count;
} CommandPaletteState;

// Initialize command palette state
void command_palette_init(CommandPaletteState* state);

// Show command palette (returns true if an action was taken)
bool command_palette_show(CommandPaletteState* state, 
                         Task* tasks, int task_count,
                         Project* projects, int project_count,
                         Context* contexts, int context_count,
                         int current_task_id);

// Open the command palette
void command_palette_open(CommandPaletteState* state);

// Close the command palette
void command_palette_close(CommandPaletteState* state);

#endif // COMMAND_PALETTE_H
