#include "undo.h"
#include "../db/database.h"
#include <string.h>

void undo_init(UndoStack* stack) {
    stack->count = 0;
    stack->current = 0;
    memset(stack->entries, 0, sizeof(stack->entries));
}

void undo_record_task_delete(UndoStack* stack, const Task* task) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        // Shift history
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_TASK_DELETE;
    entry->task_snapshot = *task;
    entry->affected_id = task->id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_task_complete(UndoStack* stack, const Task* task) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_TASK_COMPLETE;
    entry->task_snapshot = *task;
    entry->affected_id = task->id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_task_flag(UndoStack* stack, const Task* task) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_TASK_FLAG;
    entry->task_snapshot = *task;
    entry->affected_id = task->id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_task_create(UndoStack* stack, int task_id) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_TASK_CREATE;
    entry->affected_id = task_id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_task_edit(UndoStack* stack, const Task* old_task) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_TASK_EDIT;
    entry->task_snapshot = *old_task;
    entry->affected_id = old_task->id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_project_delete(UndoStack* stack, const Project* project) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_PROJECT_DELETE;
    entry->project_snapshot = *project;
    entry->affected_id = project->id;
    
    stack->count++;
    stack->current = stack->count;
}

void undo_record_project_create(UndoStack* stack, int project_id) {
    if (stack->count >= MAX_UNDO_HISTORY) {
        memmove(&stack->entries[0], &stack->entries[1], 
                sizeof(UndoEntry) * (MAX_UNDO_HISTORY - 1));
        stack->count = MAX_UNDO_HISTORY - 1;
    }
    
    UndoEntry* entry = &stack->entries[stack->count];
    entry->type = UNDO_TYPE_PROJECT_CREATE;
    entry->affected_id = project_id;
    
    stack->count++;
    stack->current = stack->count;
}

int undo_last(UndoStack* stack) {
    if (stack->count == 0 || stack->current == 0) {
        return -1;  // Nothing to undo
    }
    
    stack->current--;
    UndoEntry* entry = &stack->entries[stack->current];
    
    switch (entry->type) {
        case UNDO_TYPE_TASK_DELETE:
            // For now, undo just means recreating the task with same title
            // Full restoration would need more database support
            db_insert_task(entry->task_snapshot.title, entry->task_snapshot.status);
            return 0;
            
        case UNDO_TYPE_TASK_COMPLETE:
            // Restore completion status
            db_update_task_status(entry->affected_id, entry->task_snapshot.status);
            return 0;
            
        case UNDO_TYPE_TASK_FLAG:
            // Restore flag status
            db_update_task_flagged(entry->affected_id, entry->task_snapshot.flagged);
            return 0;
            
        case UNDO_TYPE_TASK_CREATE:
            // Delete the created task
            db_delete_task(entry->affected_id);
            return 0;
            
        case UNDO_TYPE_TASK_EDIT:
            // Restore old task title
            db_update_task_title(entry->affected_id, entry->task_snapshot.title);
            return 0;
            
        case UNDO_TYPE_PROJECT_DELETE:
            // Recreate project with same title
            db_insert_project(entry->project_snapshot.title, entry->project_snapshot.type);
            return 0;
            
        case UNDO_TYPE_PROJECT_CREATE:
            // Delete the created project
            db_delete_project(entry->affected_id);
            return 0;
            
        default:
            return -1;
    }
}

int undo_can_undo(UndoStack* stack) {
    return stack->count > 0 && stack->current > 0;
}

void undo_clear(UndoStack* stack) {
    stack->count = 0;
    stack->current = 0;
}
