#include "../test_framework.h"
#include "../../src/db/database.h"
#include "../../src/core/task.h"
#include "../../src/core/project.h"
#include "../../src/core/context.h"
#include <unistd.h>
#include <time.h>

static const char* TEST_DB_PATH = "/tmp/samfocus_test.db";

// Helper function to clean up test database
static void cleanup_test_db(void) {
    unlink(TEST_DB_PATH);
}

// Helper function to setup test database
static void setup_test_db(void) {
    cleanup_test_db();
    db_init(TEST_DB_PATH);
    db_create_schema();
}

// Helper function to teardown test database
static void teardown_test_db(void) {
    db_close();
    cleanup_test_db();
}

// ============================================================================
// Database initialization tests
// ============================================================================

TEST(test_db_init_creates_database) {
    cleanup_test_db();
    
    int result = db_init(TEST_DB_PATH);
    ASSERT_EQ(0, result, "Database initialization should succeed");
    
    // Check if file exists
    ASSERT(access(TEST_DB_PATH, F_OK) == 0, "Database file should exist");
    
    teardown_test_db();
    PASS();
}

TEST(test_db_create_schema_succeeds) {
    setup_test_db();
    
    int result = db_create_schema();
    ASSERT_EQ(0, result, "Schema creation should succeed");
    
    teardown_test_db();
    PASS();
}

// ============================================================================
// Task CRUD tests
// ============================================================================

TEST(test_insert_task_succeeds) {
    setup_test_db();
    
    int task_id = db_insert_task("Test Task", TASK_STATUS_INBOX);
    ASSERT(task_id > 0, "Task insertion should return positive ID");
    
    teardown_test_db();
    PASS();
}

TEST(test_insert_empty_task_fails) {
    setup_test_db();
    
    int task_id = db_insert_task("", TASK_STATUS_INBOX);
    ASSERT_EQ(-1, task_id, "Empty task insertion should fail");
    
    teardown_test_db();
    PASS();
}

TEST(test_load_tasks_returns_inserted_task) {
    setup_test_db();
    
    int task_id = db_insert_task("My Task", TASK_STATUS_INBOX);
    ASSERT(task_id > 0, "Task should be inserted");
    
    Task* tasks = NULL;
    int count = 0;
    int result = db_load_tasks(&tasks, &count, -1);
    
    ASSERT_EQ(0, result, "Loading tasks should succeed");
    ASSERT_EQ(1, count, "Should have 1 task");
    ASSERT_NOT_NULL(tasks, "Tasks array should not be NULL");
    ASSERT_STR_EQ("My Task", tasks[0].title, "Task title should match");
    ASSERT_EQ(TASK_STATUS_INBOX, tasks[0].status, "Task status should match");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_status) {
    setup_test_db();
    
    int task_id = db_insert_task("Task to complete", TASK_STATUS_INBOX);
    int result = db_update_task_status(task_id, TASK_STATUS_DONE);
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(TASK_STATUS_DONE, tasks[0].status, "Status should be updated");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_title) {
    setup_test_db();
    
    int task_id = db_insert_task("Old Title", TASK_STATUS_INBOX);
    int result = db_update_task_title(task_id, "New Title");
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_STR_EQ("New Title", tasks[0].title, "Title should be updated");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_notes) {
    setup_test_db();
    
    int task_id = db_insert_task("Task with notes", TASK_STATUS_INBOX);
    int result = db_update_task_notes(task_id, "These are my notes");
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_STR_EQ("These are my notes", tasks[0].notes, "Notes should be updated");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_flagged) {
    setup_test_db();
    
    int task_id = db_insert_task("Task to flag", TASK_STATUS_INBOX);
    int result = db_update_task_flagged(task_id, 1);
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(1, tasks[0].flagged, "Task should be flagged");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_defer_date) {
    setup_test_db();
    
    int task_id = db_insert_task("Task to defer", TASK_STATUS_INBOX);
    time_t defer_time = time(NULL) + 86400; // Tomorrow
    int result = db_update_task_defer_at(task_id, defer_time);
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(defer_time, tasks[0].defer_at, "Defer date should be updated");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_task_due_date) {
    setup_test_db();
    
    int task_id = db_insert_task("Task with due date", TASK_STATUS_INBOX);
    time_t due_time = time(NULL) + 172800; // 2 days from now
    int result = db_update_task_due_at(task_id, due_time);
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(due_time, tasks[0].due_at, "Due date should be updated");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_delete_task) {
    setup_test_db();
    
    int task_id = db_insert_task("Task to delete", TASK_STATUS_INBOX);
    int result = db_delete_task(task_id);
    ASSERT_EQ(0, result, "Delete should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(0, count, "Should have 0 tasks after deletion");
    
    teardown_test_db();
    PASS();
}

TEST(test_load_tasks_with_status_filter) {
    setup_test_db();
    
    db_insert_task("Inbox Task", TASK_STATUS_INBOX);
    int done_id = db_insert_task("Done Task", TASK_STATUS_INBOX);
    db_update_task_status(done_id, TASK_STATUS_DONE);
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, TASK_STATUS_INBOX);
    
    ASSERT_EQ(1, count, "Should have 1 inbox task");
    ASSERT_STR_EQ("Inbox Task", tasks[0].title, "Should be the inbox task");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

// ============================================================================
// Project tests
// ============================================================================

TEST(test_insert_project) {
    setup_test_db();
    
    int project_id = db_insert_project("My Project", PROJECT_TYPE_PARALLEL);
    ASSERT(project_id > 0, "Project insertion should return positive ID");
    
    Project* projects = NULL;
    int count = 0;
    db_load_projects(&projects, &count);
    
    ASSERT_EQ(1, count, "Should have 1 project");
    ASSERT_STR_EQ("My Project", projects[0].title, "Project title should match");
    ASSERT_EQ(PROJECT_TYPE_PARALLEL, projects[0].type, "Project type should match");
    
    free(projects);
    teardown_test_db();
    PASS();
}

TEST(test_assign_task_to_project) {
    setup_test_db();
    
    int project_id = db_insert_project("Work", PROJECT_TYPE_PARALLEL);
    int task_id = db_insert_task("Work Task", TASK_STATUS_INBOX);
    
    int result = db_assign_task_to_project(task_id, project_id);
    ASSERT_EQ(0, result, "Assignment should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(project_id, tasks[0].project_id, "Task should be assigned to project");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_update_project_title) {
    setup_test_db();
    
    int project_id = db_insert_project("Old Name", PROJECT_TYPE_PARALLEL);
    int result = db_update_project_title(project_id, "New Name");
    ASSERT_EQ(0, result, "Update should succeed");
    
    Project* projects = NULL;
    int count = 0;
    db_load_projects(&projects, &count);
    ASSERT_STR_EQ("New Name", projects[0].title, "Title should be updated");
    
    free(projects);
    teardown_test_db();
    PASS();
}

TEST(test_delete_project) {
    setup_test_db();
    
    int project_id = db_insert_project("Project to delete", PROJECT_TYPE_PARALLEL);
    int result = db_delete_project(project_id);
    ASSERT_EQ(0, result, "Delete should succeed");
    
    Project* projects = NULL;
    int count = 0;
    db_load_projects(&projects, &count);
    ASSERT_EQ(0, count, "Should have 0 projects");
    
    teardown_test_db();
    PASS();
}

// ============================================================================
// Context tests
// ============================================================================

TEST(test_insert_context) {
    setup_test_db();
    
    int context_id = db_insert_context("@home", "#FF0000");
    ASSERT(context_id > 0, "Context insertion should return positive ID");
    
    Context* contexts = NULL;
    int count = 0;
    db_load_contexts(&contexts, &count);
    
    ASSERT_EQ(1, count, "Should have 1 context");
    ASSERT_STR_EQ("@home", contexts[0].name, "Context name should match");
    
    free(contexts);
    teardown_test_db();
    PASS();
}

TEST(test_add_context_to_task) {
    setup_test_db();
    
    int task_id = db_insert_task("Task with context", TASK_STATUS_INBOX);
    int context_id = db_insert_context("@work", "#00FF00");
    
    int result = db_add_context_to_task(task_id, context_id);
    ASSERT_EQ(0, result, "Adding context should succeed");
    
    Context* contexts = NULL;
    int count = 0;
    db_get_task_contexts(task_id, &contexts, &count);
    ASSERT_EQ(1, count, "Task should have 1 context");
    ASSERT_STR_EQ("@work", contexts[0].name, "Context should match");
    
    free(contexts);
    teardown_test_db();
    PASS();
}

TEST(test_remove_context_from_task) {
    setup_test_db();
    
    int task_id = db_insert_task("Task", TASK_STATUS_INBOX);
    int context_id = db_insert_context("@errands", "#0000FF");
    db_add_context_to_task(task_id, context_id);
    
    int result = db_remove_context_from_task(task_id, context_id);
    ASSERT_EQ(0, result, "Removing context should succeed");
    
    Context* contexts = NULL;
    int count = 0;
    db_get_task_contexts(task_id, &contexts, &count);
    ASSERT_EQ(0, count, "Task should have 0 contexts");
    
    teardown_test_db();
    PASS();
}

// ============================================================================
// Recurrence tests
// ============================================================================

TEST(test_update_task_recurrence) {
    setup_test_db();
    
    int task_id = db_insert_task("Recurring Task", TASK_STATUS_INBOX);
    int result = db_update_task_recurrence(task_id, RECUR_DAILY, 1);
    ASSERT_EQ(0, result, "Update should succeed");
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(RECUR_DAILY, tasks[0].recurrence, "Recurrence should be set");
    ASSERT_EQ(1, tasks[0].recurrence_interval, "Interval should be set");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

TEST(test_create_recurring_instance) {
    setup_test_db();
    
    int task_id = db_insert_task("Daily Task", TASK_STATUS_INBOX);
    db_update_task_recurrence(task_id, RECUR_DAILY, 1);
    
    Task* tasks = NULL;
    int count = 0;
    db_load_tasks(&tasks, &count, -1);
    
    int new_task_id = db_create_recurring_instance(&tasks[0]);
    ASSERT(new_task_id > 0, "Should create new instance");
    ASSERT(new_task_id != task_id, "New instance should have different ID");
    
    free(tasks);
    
    // Load all tasks again
    db_load_tasks(&tasks, &count, -1);
    ASSERT_EQ(2, count, "Should have 2 tasks now");
    
    free(tasks);
    teardown_test_db();
    PASS();
}

// ============================================================================
// Dependency tests
// ============================================================================

TEST(test_add_dependency) {
    setup_test_db();
    
    int task1_id = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2_id = db_insert_task("Task 2", TASK_STATUS_INBOX);
    
    int result = db_add_dependency(task2_id, task1_id);
    ASSERT_EQ(0, result, "Adding dependency should succeed");
    
    int* deps = NULL;
    int dep_count = 0;
    db_get_task_dependencies(task2_id, &deps, &dep_count);
    ASSERT_EQ(1, dep_count, "Should have 1 dependency");
    ASSERT_EQ(task1_id, deps[0], "Dependency should match");
    
    free(deps);
    teardown_test_db();
    PASS();
}

TEST(test_remove_dependency) {
    setup_test_db();
    
    int task1_id = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2_id = db_insert_task("Task 2", TASK_STATUS_INBOX);
    db_add_dependency(task2_id, task1_id);
    
    int result = db_remove_dependency(task2_id, task1_id);
    ASSERT_EQ(0, result, "Removing dependency should succeed");
    
    int* deps = NULL;
    int dep_count = 0;
    db_get_task_dependencies(task2_id, &deps, &dep_count);
    ASSERT_EQ(0, dep_count, "Should have 0 dependencies");
    
    teardown_test_db();
    PASS();
}

TEST(test_is_task_blocked) {
    setup_test_db();
    
    int task1_id = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2_id = db_insert_task("Task 2", TASK_STATUS_INBOX);
    db_add_dependency(task2_id, task1_id);
    
    int blocked = db_is_task_blocked(task2_id);
    ASSERT_EQ(1, blocked, "Task 2 should be blocked");
    
    // Complete task 1
    db_update_task_status(task1_id, TASK_STATUS_DONE);
    
    blocked = db_is_task_blocked(task2_id);
    ASSERT_EQ(0, blocked, "Task 2 should not be blocked anymore");
    
    teardown_test_db();
    PASS();
}

TEST(test_self_dependency_fails) {
    setup_test_db();
    
    int task_id = db_insert_task("Task", TASK_STATUS_INBOX);
    int result = db_add_dependency(task_id, task_id);
    ASSERT_EQ(-1, result, "Self-dependency should fail");
    
    teardown_test_db();
    PASS();
}

// ============================================================================
// Main test runner
// ============================================================================

int main(void) {
    TEST_SUITE("Database Tests");
    
    // Initialization tests
    RUN_TEST(test_db_init_creates_database);
    RUN_TEST(test_db_create_schema_succeeds);
    
    // Task CRUD tests
    RUN_TEST(test_insert_task_succeeds);
    RUN_TEST(test_insert_empty_task_fails);
    RUN_TEST(test_load_tasks_returns_inserted_task);
    RUN_TEST(test_update_task_status);
    RUN_TEST(test_update_task_title);
    RUN_TEST(test_update_task_notes);
    RUN_TEST(test_update_task_flagged);
    RUN_TEST(test_update_task_defer_date);
    RUN_TEST(test_update_task_due_date);
    RUN_TEST(test_delete_task);
    RUN_TEST(test_load_tasks_with_status_filter);
    
    // Project tests
    RUN_TEST(test_insert_project);
    RUN_TEST(test_assign_task_to_project);
    RUN_TEST(test_update_project_title);
    RUN_TEST(test_delete_project);
    
    // Context tests
    RUN_TEST(test_insert_context);
    RUN_TEST(test_add_context_to_task);
    RUN_TEST(test_remove_context_from_task);
    
    // Recurrence tests
    RUN_TEST(test_update_task_recurrence);
    RUN_TEST(test_create_recurring_instance);
    
    // Dependency tests
    RUN_TEST(test_add_dependency);
    RUN_TEST(test_remove_dependency);
    RUN_TEST(test_is_task_blocked);
    RUN_TEST(test_self_dependency_fails);
    
    PRINT_TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
