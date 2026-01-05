# SamFocus Testing Documentation

## Overview

SamFocus includes a comprehensive testing suite with unit tests and integration tests covering all major functionality.

## Test Statistics

- **Total Tests**: 36
- **Unit Tests**: 26
- **Integration Tests**: 10
- **Coverage**: Core database operations, task management, projects, contexts, recurrence, and dependencies

## Running Tests

### Quick Test Run

```bash
./run_tests.sh
```

### Verbose Output

```bash
./run_tests.sh -v
```

### Using Meson Directly

```bash
# Run all tests
meson test -C build

# Run with verbose output
meson test -C build -v

# Run specific test suite
meson test -C build "Database Unit Tests"
meson test -C build "Integration Workflow Tests"
```

## Test Structure

```
tests/
├── test_framework.h          # Custom testing framework
├── unit/
│   └── test_database.c       # Database operation unit tests
└── integration/
    └── test_workflows.c      # End-to-end workflow tests
```

## Test Framework Features

Our custom test framework (`test_framework.h`) provides:

- **Test Macros**:
  - `TEST(name)` - Define a test case
  - `ASSERT(condition, message)` - Assert a condition is true
  - `ASSERT_EQ(expected, actual, message)` - Assert equality
  - `ASSERT_STR_EQ(expected, actual, message)` - Assert string equality
  - `ASSERT_NOT_NULL(ptr, message)` - Assert pointer is not NULL
  - `ASSERT_NULL(ptr, message)` - Assert pointer is NULL
  - `PASS()` - Mark test as passed

- **Test Organization**:
  - `TEST_SUITE(name)` - Start a test suite
  - `RUN_TEST(test)` - Execute a test
  - `PRINT_TEST_SUMMARY()` - Print results

- **Color-Coded Output**:
  - Green for passed tests
  - Red for failed tests
  - Blue for test names
  - Yellow for summaries

## Unit Tests Coverage

### Database Operations (26 tests)

#### Initialization
- Database creation and file existence
- Schema creation with tables and indexes

#### Task CRUD
- Insert task (valid and invalid)
- Load tasks with and without filters
- Update task status, title, notes, flags
- Update defer and due dates
- Delete tasks
- Task ordering

#### Projects
- Insert projects
- Assign tasks to projects
- Update project properties
- Delete projects with cascading

#### Contexts
- Insert contexts
- Add/remove contexts from tasks
- Load task contexts

#### Recurrence
- Update task recurrence settings
- Create recurring instances
- Preserve task properties in instances

#### Dependencies
- Add dependencies between tasks
- Remove dependencies
- Check if tasks are blocked
- Prevent self-dependencies

## Integration Tests Coverage

### Complete Workflows (10 tests)

1. **Complete GTD Workflow**
   - Create inbox tasks
   - Organize into projects
   - Add contexts
   - Set priorities and due dates
   - Complete tasks

2. **Sequential Project Workflow**
   - Create sequential project
   - Add ordered tasks
   - Get first incomplete task
   - Progress through sequence

3. **Recurring Task Workflow**
   - Create recurring task
   - Complete and spawn next instance
   - Verify property preservation

4. **Dependency Chain Workflow**
   - Create task chain with dependencies
   - Verify blocking behavior
   - Complete tasks and unblock

5. **Batch Operations Workflow**
   - Create multiple tasks
   - Perform batch flag/complete operations
   - Verify results

6. **Defer and Due Date Workflow**
   - Create tasks with various dates
   - Query and filter by dates

7. **Multi-Context Workflow**
   - Add multiple contexts to tasks
   - Query contexts

8. **Project Deletion Cascades**
   - Delete projects
   - Verify tasks remain with NULL project_id

9. **Task Deletion Removes Dependencies**
   - Create dependencies
   - Delete tasks
   - Verify cascade deletion

10. **Order Index Workflow**
    - Set task ordering
    - Reorder tasks
    - Verify persistence

## Continuous Integration

### GitHub Actions

The project includes CI/CD configuration in `.github/workflows/ci.yml`:

- **Linux Testing**: Full test suite on Ubuntu
- **Windows Building**: Compilation on Windows with MSYS2
- **Code Quality**: Static analysis with cppcheck

### CI Workflow Triggers

- Push to `main` or `develop` branches
- Pull requests to `main` or `develop`

### Build Matrix

- **Linux**: Ubuntu latest with GCC
- **Windows**: Windows latest with MinGW-w64

## Adding New Tests

### Unit Test Example

```c
TEST(test_my_feature) {
    setup_test_db();
    
    // Arrange
    int id = db_insert_task("Test", TASK_STATUS_INBOX);
    
    // Act
    int result = db_my_operation(id);
    
    // Assert
    ASSERT_EQ(0, result, "Operation should succeed");
    
    teardown_test_db();
    PASS();
}
```

### Integration Test Example

```c
TEST(test_my_workflow) {
    setup_test_db();
    
    // Create test data
    int task1 = db_insert_task("Task 1", TASK_STATUS_INBOX);
    int task2 = db_insert_task("Task 2", TASK_STATUS_INBOX);
    
    // Perform workflow operations
    db_add_dependency(task2, task1);
    db_update_task_status(task1, TASK_STATUS_DONE);
    
    // Verify end state
    int blocked = db_is_task_blocked(task2);
    ASSERT_EQ(0, blocked, "Task should not be blocked");
    
    teardown_test_db();
    PASS();
}
```

### Register Test in meson.build

```meson
test_my_feature = executable('test_my_feature',
  'tests/unit/test_my_feature.c',
  test_db_sources,
  include_directories: [src_inc, include_directories('tests')],
  dependencies: [sqlite_dep],
)

test('My Feature Tests', test_my_feature)
```

## Test Database Management

Tests use isolated database files:
- Unit tests: `/tmp/samfocus_test.db`
- Integration tests: `/tmp/samfocus_integration_test.db`

Each test:
1. **Setup**: Creates fresh database
2. **Execute**: Runs test operations
3. **Teardown**: Closes and deletes database

This ensures test isolation and prevents interference.

## Debugging Failed Tests

### View Test Logs

```bash
cat build/meson-logs/testlog.txt
```

### Run Single Test

```bash
./build/test_database
./build/test_workflows
```

### Enable Verbose Mode

```bash
meson test -C build -v
```

### Common Issues

1. **Database lock errors**: Another process has the test database open
   - Solution: Close all SamFocus instances

2. **Permission errors**: Can't write to /tmp
   - Solution: Check /tmp permissions

3. **Compilation errors**: Missing dependencies
   - Solution: Install required packages (see README.md)

## Test Maintenance

### When to Update Tests

- Adding new database operations
- Changing existing APIs
- Adding new features
- Fixing bugs (add regression test)

### Test Guidelines

1. **One Assertion Per Test**: Tests should verify one thing
2. **Descriptive Names**: Use clear test function names
3. **Clean Setup/Teardown**: Always clean up resources
4. **Isolated Tests**: Tests should not depend on each other
5. **Fast Execution**: Keep tests quick (<100ms each)

## Performance Benchmarks

Current test execution times:
- Unit tests: ~80ms
- Integration tests: ~60ms
- Total: ~140ms

Target: Keep total test time under 500ms for fast feedback.

## Future Test Additions

Planned test coverage expansions:

- [ ] Export/backup functionality tests
- [ ] CLI tool integration tests
- [ ] Markdown rendering tests
- [ ] Command palette functionality tests
- [ ] UI component tests (if feasible)
- [ ] Performance/stress tests
- [ ] Memory leak detection tests

## Contributing Tests

When contributing:

1. Write tests for new features
2. Update existing tests if behavior changes
3. Ensure all tests pass before submitting PR
4. Add integration test for complete workflows
5. Document any new test patterns

## Test Coverage Goals

- **Database Layer**: 100% (achieved ✓)
- **Core Logic**: 95% (achieved ✓)
- **UI Layer**: 30% (manual testing primarily)
- **CLI Tool**: 80% (planned)
- **Export/Backup**: 90% (planned)

---

For questions or issues with tests, please open a GitHub issue.
