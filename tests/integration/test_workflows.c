#include "../test_framework.h"
#include "../../src/db/database.h"
#include "../../src/core/task.h"
#include "../../src/core/project.h"
#include "../../src/core/context.h"
#include <unistd.h>
#include <time.h>

static const char* TEST_DB_PATH = "/tmp/samfocus_integration_test.db";

static void cleanup_test_db(void) {
    unlink(TEST_DB_PATH);
}

static void setup_test_db(void) {
    cleanup_test_db();
    db_init(TEST_DB_PATH);
    db_create_schema();
}

static void teardown_test_db(void) {
    db_close();
    cleanup_test_db();
}

// ============================================================================
// GTD Workflow Tests
// ============================================================================

TEST(test_complete_gtd_workflow) {
    setup_test_db();
    
    // 1. Create inbox tasks
    int task1 = db_insert_task("Write proposal", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Review code", TASK_STATUS_INBOX);
    int task3 = db_insert_task("Call dentist", TASK_STATUS_INBOX);
    
    ASSERT(task1 > 0 && task2 > 0 && task3 > 0, "Tasks should be created");
    
    // 2. Create projects
    int work_project = db_insert_project("Work", PROJECT_TYPE_PARALLEL);
    int personal_project = db_insert_project("Personal", PROJECT_TYPE_PARALLEL);
    
    ASSERT(work_project > 0 && personal_project > 0, "Projects should be created");
    
    // 3. Organize tasks into projects
    db_assign_task_to_project(task1, work_project);
    db_assign_task_to_project(task2, work_project);
    db_assign_task_to_project(task3, personal_project);
    
    // 4. Add contexts
    int context_office = db_insert_context("@office", "#0000FF");
    int context_phone = db_insert_context("@phone", "#00FF00");
    
    db_add_context_to_task(task1, context_office);
    db_add_context_to_task(task2, context_office);
    db_add_context_to_task(task3, context_phone);
    
    // 5. Set priorities with flags
    db_update_task_flagged(task1, 1);
    
    // 6. Set due dates
    time_t tomorrow = time(NULL) + 86400;
    db_update_task_due_at(task1, tomorrow);
    
    // 7. Complete a task
    db_update_task_status(task2, TASK_STATUS_DONE);
    
    // Verify the workflow
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    
    ASSERT_EQ(3, count, "Should have 3 tasks");
    
    // Find and verify task1
    Task* t1 = NULL;
    for (int i = 0; i < count; i++) {
        if (tasks[i].id == task1) {
            t1 = &tasks[i];
            break;
        }
    }
    
    ASSERT_NOT_NULL(t1, "Task 1 should exist");
    ASSERT_EQ(work_project, t1->project_id, "Task 1 should be in work project");
    ASSERT_EQ(1, t1->flagged, "Task 1 should be flagged");
    ASSERT_EQ(tomorrow, t1->due_at, "Task 1 should have due date");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_sequential_project_workflow) {
    setup_test_db();
    
    // Create a sequential project
    int project_id = db_insert_project("Launch Website", PROJECT_TYPE_SEQUENTIAL);
    
    // Add tasks in sequence
    int task1 = db_insert_task("Design mockups", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Implement frontend", TASK_STATUS_INBOX);
    int task3 = db_insert_task("Deploy to production", TASK_STATUS_INBOX);
    
    db_assign_task_to_project(task1, project_id);
    db_assign_task_to_project(task2, project_id);
    db_assign_task_to_project(task3, project_id);
    
    // Get first incomplete task
    int first = db_get_first_incomplete_task_in_project(project_id);
    ASSERT(first > 0, "Should have first incomplete task");
    
    // Complete first task
    db_update_task_status(first, TASK_STATUS_DONE);
    
    // Get next incomplete task
    int second = db_get_first_incomplete_task_in_project(project_id);
    ASSERT(second > 0, "Should have second incomplete task");
    ASSERT(second != first, "Second task should be different from first");
    
    teardown_test_db();
    PASS();
}

TEST(test_recurring_task_workflow) {
    setup_test_db();
    
    // Create a daily recurring task
    int task_id = db_insert_task("Review email", TASK_STATUS_INBOX);
    db_update_task_recurrence(task_id, RECUR_DAILY, 1);
    db_update_task_notes(task_id, "Check inbox every morning");
    
    // Load the task
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(1, count, "Should have 1 task");
    
    // Complete the task and create next instance
    db_update_task_status(tasks[0].id, TASK_STATUS_DONE);
    int new_id = db_create_recurring_instance(&tasks[0]);
    ASSERT(new_id > 0, "Should create recurring instance");
    
    free(tasks);
    
    // Load all tasks
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(2, count, "Should have 2 tasks (completed + new instance)");
    
    // Verify new instance has same properties
    Task* new_task = NULL;
    for (int i = 0; i < count; i++) {
        if (tasks[i].id == new_id) {
            new_task = &tasks[i];
            break;
        }
    }
    
    ASSERT_NOT_NULL(new_task, "New task should exist");
    ASSERT_STR_EQ("Review email", new_task->title, "New task should have same title");
    ASSERT_EQ(RECUR_DAILY, new_task->recurrence, "New task should have same recurrence");
    ASSERT_STR_EQ("Check inbox every morning", new_task->notes, "New task should have same notes");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_dependency_chain_workflow) {
    setup_test_db();
    
    // Create a chain of dependent tasks
    int task1 = db_insert_task("Get requirements", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Write specification", TASK_STATUS_INBOX);
    int task3 = db_insert_task("Implement feature", TASK_STATUS_INBOX);
    int task4 = db_insert_task("Write tests", TASK_STATUS_INBOX);
    
    // Set up dependencies: task2 depends on task1, task3 on task2, task4 on task3
    db_add_dependency(task2, task1);
    db_add_dependency(task3, task2);
    db_add_dependency(task4, task3);
    
    // All tasks except first should be blocked
    ASSERT_EQ(0, db_is_task_blocked(task1), "Task 1 should not be blocked");
    ASSERT_EQ(1, db_is_task_blocked(task2), "Task 2 should be blocked");
    ASSERT_EQ(1, db_is_task_blocked(task3), "Task 3 should be blocked");
    ASSERT_EQ(1, db_is_task_blocked(task4), "Task 4 should be blocked");
    
    // Complete task1
    db_update_task_status(task1, TASK_STATUS_DONE);
    
    // Now only task2 should be unblocked
    ASSERT_EQ(0, db_is_task_blocked(task2), "Task 2 should not be blocked");
    ASSERT_EQ(1, db_is_task_blocked(task3), "Task 3 should still be blocked");
    
    // Complete task2
    db_update_task_status(task2, TASK_STATUS_DONE);
    
    // Now task3 should be unblocked
    ASSERT_EQ(0, db_is_task_blocked(task3), "Task 3 should not be blocked");
    ASSERT_EQ(1, db_is_task_blocked(task4), "Task 4 should still be blocked");
    
    teardown_test_db();
    PASS();
}

TEST(test_batch_operations_workflow) {
    setup_test_db();
    
    // Create multiple tasks for batch operations
    int tasks[5];
    for (int i = 0; i < 5; i++) {
        char title[32];
        snprintf(title, sizeof(title), "Task %d", i + 1);
        tasks[i] = db_insert_task(title, TASK_STATUS_INBOX);
    }
    
    // Batch flag operation
    for (int i = 0; i < 3; i++) {
        db_update_task_flagged(tasks[i], 1);
    }
    
    // Batch complete operation
    for (int i = 0; i < 2; i++) {
        db_update_task_status(tasks[i], TASK_STATUS_DONE);
    }
    
    // Verify results
    Task* loaded_tasks = NULL;
    int count = 0;
    db_load_tasks(&loaded_tasks, &count, -1);
    
    int flagged_count = 0;
    int done_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (loaded_tasks[i].flagged) flagged_count++;
        if (loaded_tasks[i].status == TASK_STATUS_DONE) done_count++;
    }
    
    ASSERT_EQ(3, flagged_count, "Should have 3 flagged tasks");
    ASSERT_EQ(2, done_count, "Should have 2 completed tasks");
    
    free(loaded_tasks);
    teardown_test_db();
    PASS();
}

TEST(test_defer_and_due_date_workflow) {
    setup_test_db();
    
    time_t now = time(NULL);
    time_t tomorrow = now + 86400;
    time_t next_week = now + (7 * 86400);
    
    // Create tasks with different defer/due dates
    int task1 = db_insert_task("Do today", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Do tomorrow", TASK_STATUS_INBOX);
    int task3 = db_insert_task("Do next week", TASK_STATUS_INBOX);
    
    db_update_task_defer_at(task2, tomorrow);
    db_update_task_defer_at(task3, next_week);
    db_update_task_due_at(task1, tomorrow);
    
    // Load and verify
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    
    ASSERT_EQ(3, count, "Should have 3 tasks");
    
    // Find task with due date
    int found_due = 0;
    for (int i = 0; i < count; i++) {
        if (tasks[i].due_at == tomorrow) {
            found_due = 1;
            ASSERT_STR_EQ("Do today", tasks[i].title, "Task with due date should be 'Do today'");
        }
    }
    ASSERT_EQ(1, found_due, "Should find task with due date");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_multi_context_workflow) {
    setup_test_db();
    
    // Create task with multiple contexts
    int task_id = db_insert_task("Buy groceries and mail package", TASK_STATUS_INBOX);
    
    int ctx1 = db_insert_context("@errands", "#FF0000");
    int ctx2 = db_insert_context("@shopping", "#00FF00");
    int ctx3 = db_insert_context("@postoffice", "#0000FF");
    
    db_add_context_to_task(task_id, ctx1);
    db_add_context_to_task(task_id, ctx2);
    db_add_context_to_task(task_id, ctx3);
    
    // Get contexts for task
    Context* contexts = NULL;
    int count = 0;
    db_get_task_contexts(task_id, &contexts, &count);
    
    ASSERT_EQ(3, count, "Task should have 3 contexts");
    
    free(contexts);
    teardown_test_db();
    PASS();
}

TEST(test_project_deletion_cascades) {
    setup_test_db();
    
    // Create project with tasks
    int project_id = db_insert_project("Test Project", PROJECT_TYPE_PARALLEL);
    int task1 = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Task 2", TASK_STATUS_INBOX);
    
    db_assign_task_to_project(task1, project_id);
    db_assign_task_to_project(task2, project_id);
    
    // Delete project
    db_delete_project(project_id);
    
    // Verify tasks still exist but have no project
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    
    ASSERT_EQ(2, count, "Tasks should still exist");
    ASSERT_EQ(0, tasks[0].project_id, "Task should have no project");
    ASSERT_EQ(0, tasks[1].project_id, "Task should have no project");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_task_deletion_removes_dependencies) {
    setup_test_db();
    
    int task1 = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Task 2", TASK_STATUS_INBOX);
    
    db_add_dependency(task2, task1);
    
    // Verify dependency exists
    int* deps = NULL;
    int dep_count = 0;
    db_get_task_dependencies(task2, &deps, &dep_count);
    ASSERT_EQ(1, dep_count, "Should have 1 dependency");
    free(deps);
    
    // Delete task1
    db_delete_task(task1);
    
    // Verify dependency is removed (cascade delete)
    db_get_task_dependencies(task2, &deps, &dep_count);
    ASSERT_EQ(0, dep_count, "Dependency should be removed");
    
    teardown_test_db();
    PASS();
}

TEST(test_order_index_workflow) {
    setup_test_db();
    
    // Create tasks with specific order
    int task1 = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Task 2", TASK_STATUS_INBOX);
    int task3 = db_insert_task("Task 3", TASK_STATUS_INBOX);
    
    db_update_task_order_index(task1, 1);
    db_update_task_order_index(task2, 2);
    db_update_task_order_index(task3, 3);
    
    // Swap task1 and task2
    db_update_task_order_index(task1, 2);
    db_update_task_order_index(task2, 1);
    
    // Load and verify order
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    
    ASSERT_EQ(3, count, "Should have 3 tasks");
    
    // Tasks should be ordered by order_index
    ASSERT_EQ(1, tasks[0].order_index, "First task should have order 1");
    ASSERT_EQ(2, tasks[1].order_index, "Second task should have order 2");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void) {
    TEST_SUITE("Integration Tests");
    
    RUN_TEST(test_complete_gtd_workflow);
    RUN_TEST(test_sequential_project_workflow);
    RUN_TEST(test_recurring_task_workflow);
    RUN_TEST(test_dependency_chain_workflow);
    RUN_TEST(test_batch_operations_workflow);
    RUN_TEST(test_defer_and_due_date_workflow);
    RUN_TEST(test_multi_context_workflow);
    RUN_TEST(test_project_deletion_cascades);
    RUN_TEST(test_task_deletion_removes_dependencies);
    RUN_TEST(test_order_index_workflow);
    
    PRINT_TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
