# AGENTS.md - Developer Guide for SamFocus

This guide is for agentic coding assistants working in the SamFocus codebase. SamFocus is a cross-platform GTD task management application written in C using Dear ImGui (cimgui), GLFW, and SQLite.

## Build System

**Build tool:** Zig (0.14.0+)  
**C Standard:** C11  
**C++ Standard:** C++11 (for Dear ImGui only)

### Common Commands

```bash
# Build everything (GUI app, CLI tool, tests)
zig build

# Run application
./zig-out/bin/samfocus

# Run CLI tool
./zig-out/bin/samfocus-cli help

# Clean and rebuild
rm -rf zig-out .zig-cache
zig build

# Cross-compile for Windows (from any platform)
zig build -Dtarget=x86_64-windows-gnu
```

### Testing

```bash
# Run all tests (clean output)
zig build test

# Run specific test executables
zig build test-db         # Unit tests only
zig build test-workflows  # Integration tests only

# Run test executables directly
./zig-out/bin/test_database   # Unit tests
./zig-out/bin/test_workflows  # Integration tests
```

**Test framework:** Custom framework in `tests/test_framework.h`  
**Test files:**
- `tests/unit/test_database.c` - 26 unit tests for database operations
- `tests/integration/test_workflows.c` - 10 integration tests for complete workflows

## Code Organization

```
src/
├── main.c                    # Application entry point with ImGui setup
├── core/                     # Core data structures and business logic
│   ├── task.c/h              # Task struct and enums
│   ├── project.c/h           # Project struct and types
│   ├── context.c/h           # Context/tag system
│   ├── undo.c/h              # Undo/redo system
│   ├── export.c/h            # Export/backup functionality
│   ├── platform.c/h          # Cross-platform utilities (paths, etc.)
│   └── preferences.c/h       # User preferences management
├── db/                       # Database layer (SQLite)
│   └── database.c/h          # All database operations
├── ui/                       # User interface components
│   ├── inbox_view.c/h        # Main task list view
│   ├── sidebar.c/h           # Navigation sidebar
│   ├── help_overlay.c/h      # Help system (? key)
│   ├── command_palette.c/h   # Command palette (Ctrl+K)
│   ├── launcher.c/h          # Quick launcher (Ctrl+Space)
│   └── markdown.c/h          # Markdown renderer for notes
└── cli/                      # CLI companion tool
    └── samfocus-cli.c        # Standalone CLI executable
```

## Code Style Guidelines

### Header Files

**Pattern:** All headers must have include guards:

```c
#ifndef FILENAME_H
#define FILENAME_H

// declarations

#endif // FILENAME_H
```

### Imports/Includes

**Order:** System headers first, then project headers, always alphabetized:

```c
#include "header_for_this_file.h"  // Own header first (in .c files)
#include "../db/database.h"        // Project headers
#include "../core/task.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS  // Must be before cimgui.h
#include <cimgui.h>                // External libraries
#include <stdio.h>                 // System headers
#include <stdlib.h>
#include <string.h>
```

**Important:** Always include `#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS` before `#include <cimgui.h>`

### Naming Conventions

- **Functions:** `snake_case` with module prefix: `db_insert_task()`, `launcher_show()`, `task_create()`
- **Structs:** `PascalCase`: `Task`, `Project`, `Context`, `LauncherAction`
- **Enums:** `PascalCase` for type, `UPPER_SNAKE_CASE` for values: `TaskStatus`, `TASK_STATUS_INBOX`
- **Constants:** `UPPER_SNAKE_CASE`: `MAX_RESULTS`, `INPUT_BUF_SIZE`
- **Static variables:** `snake_case`: `launcher_visible`, `input_buffer`
- **Macros:** `UPPER_SNAKE_CASE`: `TEST()`, `ASSERT()`

### Types

```c
// Use typedef for structs and enums
typedef struct {
    int id;
    char title[256];
    TaskStatus status;
} Task;

typedef enum {
    TASK_STATUS_INBOX = 0,
    TASK_STATUS_ACTIVE = 1,
    TASK_STATUS_DONE = 2
} TaskStatus;
```

### Error Handling

**Pattern:** Return `0` on success, `-1` on error. Use static error buffer for messages:

```c
static char error_msg[512] = {0};

static void set_error(const char* msg) {
    snprintf(error_msg, sizeof(error_msg), "%s", msg);
}

const char* db_get_error(void) {
    return error_msg;
}

int db_insert_task(const char* title, TaskStatus status) {
    if (db == NULL) {
        set_error("Database not initialized");
        return -1;
    }
    // ... perform operation
    return 0;  // success
}
```

### Documentation

**Use Doxygen-style comments for public functions:**

```c
/**
 * Insert a new task.
 * 
 * @param title Task title (must not be empty)
 * @param status Initial task status
 * 
 * Returns the new task ID on success, -1 on error.
 */
int db_insert_task(const char* title, TaskStatus status);
```

### Memory Management

- **Always free allocated memory:** Use `free()` for malloc/calloc allocations
- **Pattern for database queries:** Caller must free arrays returned from `db_load_*()` functions
- **Static buffers:** Prefer fixed-size static buffers for UI strings (256 chars for titles, 512 for input)

## Testing Conventions

### Test Structure

```c
TEST(test_name_describes_behavior) {
    // Setup
    setup_test_db();
    
    // Exercise
    int result = db_insert_task("Test", TASK_STATUS_INBOX);
    
    // Assert
    ASSERT(result > 0, "Task insertion should return positive ID");
    
    // Teardown
    teardown_test_db();
    PASS();
}
```

### Test Macros

- `TEST(name)` - Define a test
- `ASSERT(condition, msg)` - Assert condition is true
- `ASSERT_EQ(expected, actual, msg)` - Assert equality
- `ASSERT_STR_EQ(expected, actual, msg)` - Assert string equality
- `ASSERT_NOT_NULL(ptr, msg)` - Assert pointer is not NULL
- `PASS()` - Mark test as passed (required at end of each test)
- `RUN_TEST(test)` - Run a test in main()

## Platform-Specific Code

Use preprocessor directives for platform differences:

```c
#ifdef PLATFORM_LINUX
    // Linux-specific code
#elif defined(PLATFORM_WINDOWS)
    // Windows-specific code
#endif
```

Platform macros are defined in `build.zig` via `-DPLATFORM_LINUX` or `-DPLATFORM_WINDOWS`.

## Important Notes

1. **Never commit secrets:** `.db` files should be in `.gitignore`
2. **UI state:** Use static variables in `.c` files for component state (not globals in headers)
3. **Database transactions:** All write operations update `modified_at` timestamp
4. **Foreign keys:** Always enabled via `PRAGMA foreign_keys = ON` in `db_init()`
5. **Indices:** All commonly queried columns have indices (see `db_create_schema()`)
6. **Test isolation:** Each test should setup/teardown its own database (`/tmp/samfocus_test.db`)

## Adding New Features

### Checklist for new features:

1. Create header file in appropriate directory (`src/core/`, `src/ui/`, etc.)
2. Implement in corresponding `.c` file
3. Add files to `app_sources` in `build.zig`
4. Include header in `main.c` if needed
5. Add keyboard shortcuts to `help_overlay.c`
6. Write unit tests in `tests/unit/` and/or integration tests in `tests/integration/`
7. Run `zig build test` to verify all tests pass
8. Update documentation (README.md, LAUNCHER.md, etc.)
9. Commit with descriptive message following project convention

### Database Schema Changes:

1. Update schema in `db_create_schema()` using `CREATE TABLE IF NOT EXISTS`
2. Add migration code using `ALTER TABLE` (wrap in transaction, suppress errors if column exists)
3. Create accessor functions in `database.c` following naming pattern `db_<action>_<entity>`
4. Add function declarations to `database.h` with Doxygen comments
5. Write unit tests for new database operations

## Current Status

- **37 features** implemented
- **36 tests** (26 unit + 10 integration)
- **100% test pass rate**
- **Production ready**
