#ifndef UNDO_H
#define UNDO_H

#include "task.h"
#include "project.h"

#define MAX_UNDO_HISTORY 50

typedef enum {
    UNDO_TYPE_NONE = 0,
    UNDO_TYPE_TASK_DELETE,
    UNDO_TYPE_TASK_COMPLETE,
    UNDO_TYPE_TASK_FLAG,
    UNDO_TYPE_TASK_CREATE,
    UNDO_TYPE_TASK_EDIT,
    UNDO_TYPE_PROJECT_DELETE,
    UNDO_TYPE_PROJECT_CREATE
} UndoType;

typedef struct {
    UndoType type;
    Task task_snapshot;        // Full task state before change
    Project project_snapshot;  // Full project state before change
    int affected_id;           // ID of affected object
} UndoEntry;

typedef struct {
    UndoEntry entries[MAX_UNDO_HISTORY];
    int count;
    int current;  // Current position in history
} UndoStack;

// Initialize the undo system
void undo_init(UndoStack* stack);

// Record an action that can be undone
void undo_record_task_delete(UndoStack* stack, const Task* task);
void undo_record_task_complete(UndoStack* stack, const Task* task);
void undo_record_task_flag(UndoStack* stack, const Task* task);
void undo_record_task_create(UndoStack* stack, int task_id);
void undo_record_task_edit(UndoStack* stack, const Task* old_task);
void undo_record_project_delete(UndoStack* stack, const Project* project);
void undo_record_project_create(UndoStack* stack, int project_id);

// Perform undo
int undo_last(UndoStack* stack);

// Check if undo is available
int undo_can_undo(UndoStack* stack);

// Clear undo history
void undo_clear(UndoStack* stack);

#endif // UNDO_H
