# SamFocus

A fully-featured, cross-platform GTD task management application built with C, Dear ImGui, and SQLite.

**Status:** 37 major features complete | 36 automated tests (100% passing) | Production-ready

## Features

### Core GTD Features
- **Inbox**: Quick capture with zero friction
- **Projects**: Parallel & sequential project types
- **Contexts/Tags**: Organize by location, tool, or energy level (@home, @work, etc.)
- **Next Actions**: Smart view showing only available tasks
- **Flagged/Starred**: Priority task marking
- **Defer Dates**: Hide tasks until they're relevant
- **Due Dates**: Visual indicators for urgency
- **Rich Notes**: Markdown editor with preview mode
- **Recurring Tasks**: Daily, weekly, monthly, yearly repetition

### Advanced Features
- **Task Dependencies**: Block tasks until prerequisites complete
- **Batch Operations**: Multi-select with checkboxes for bulk actions
- **Manual Ordering**: Drag or use arrow buttons to reorder
- **Review Mode**: See recently modified tasks
- **Search/Filter**: Find tasks quickly
- **Command Palette** (Ctrl+K): Quick access to all actions
- **Undo System** (Ctrl+Z): Revert accidental changes

### Raycast-Style Quick Launcher (Ctrl+Space)
- **Natural Language Date Parsing**: "tomorrow", "next monday", "in 2 weeks"
- **Smart Keyword Extraction**: `!` for flags, `@` for contexts, `#` for projects
- **Built-in Calculator**: Type "2+2" for instant results
- **Fuzzy Task Search**: Find existing tasks quickly
- **Prefix System**: `/` for commands, `@` for contexts, `#` for projects

### UI/UX
- **Keyboard-First**: Every operation accessible via keyboard
- **Mouse Support**: Optional point-and-click interface
- **Help Overlay** (?): Comprehensive keyboard shortcut reference
- **Customizable Hotkeys**: Configure launcher key in preferences
- **Color-Coded**: Visual indicators (🔄 recurring, ⏳ blocked, ★ flagged)

### Data & Export
- **SQLite Database**: Fast, reliable local storage
- **Export System**: Text, Markdown, CSV formats
- **Database Backup**: One-click backups with timestamps
- **CLI Tool**: Command-line companion (`samfocus-cli`)
- **Cross-Platform**: Linux and Windows support
- **Local-First**: No sync, no cloud, your data stays with you

## Building

### Dependencies

#### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
    meson \
    ninja-build \
    libglfw3-dev \
    libsqlite3-dev \
    libgl1-mesa-dev \
    pkg-config \
    gcc \
    g++
```

#### Linux (Fedora/RHEL)

```bash
sudo dnf install -y \
    meson \
    ninja-build \
    glfw-devel \
    sqlite-devel \
    mesa-libGL-devel \
    pkg-config \
    gcc \
    gcc-c++
```

#### Linux (Arch)

```bash
sudo pacman -S \
    meson \
    ninja \
    glfw \
    sqlite \
    mesa \
    pkg-config \
    gcc
```

#### Windows

1. Install [Python](https://www.python.org/downloads/) (for meson)
2. Install meson: `pip install meson ninja`
3. Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ tools or [MSYS2](https://www.msys2.org/) with MinGW-w64
4. Dependencies (GLFW, SQLite) will be handled by meson automatically

**Note:** The Windows build uses static linking by default to create standalone executables that don't require external DLLs. This ensures the application runs on any Windows system without dependency issues.

### Build Instructions

```bash
# Clone the repository (if not already cloned)
git clone <your-repo-url>
cd samfocus

# Initialize submodules
git submodule update --init --recursive

# Configure the build
meson setup build

# Compile
meson compile -C build

# Run
./build/samfocus  # On Linux/Mac
# or
build\samfocus.exe  # On Windows
```

### Static Linking (Windows)

On Windows, the build system automatically creates statically-linked executables to avoid DLL dependency issues. This means:

- The executable includes all required libraries
- No external DLLs are needed (except system DLLs like kernel32.dll)
- The application runs on any Windows system without installing dependencies
- The executable size is larger but more portable

On Linux, dynamic linking is used by default as it's the standard practice and integrates better with system libraries.

### Development Build

For faster iteration during development:

```bash
meson setup build --buildtype=debug
meson compile -C build && ./build/samfocus
```

## Usage

### Keyboard Shortcuts

**Quick Access:**
- **Ctrl+Space**: Toggle Quick Launcher (customizable in preferences)
- **Ctrl+K**: Open Command Palette
- **Ctrl+,**: Open Preferences
- **Ctrl+N**: Focus new task input
- **Ctrl+Z**: Undo last action
- **? or Shift+/**: Toggle help overlay

**Perspectives:**
- **Ctrl+1**: Today (due today or overdue)
- **Ctrl+2**: Anytime (available tasks)
- **Ctrl+3**: Flagged (starred tasks)
- **Ctrl+4**: Inbox (uncategorized)
- **Ctrl+5**: Completed (done tasks)

**Task Navigation:**
- **Up/Down Arrow**: Navigate through task list
- **Home**: Jump to first task
- **End**: Jump to last task

**Task Actions:**
- **Enter**: Edit selected task or add new task (in input field)
- **Space**: Toggle completion status
- **F**: Flag/unflag selected task
- **Delete**: Remove selected task
- **Escape**: Cancel edit mode or close overlays

**Mouse Actions:**
- **Click task**: Edit task
- **Checkbox**: Toggle completion
- **Star icon**: Flag task
- **Delete button**: Remove task

### Quick Launcher Examples

Press **Ctrl+Space** to open the launcher, then try:

- `Buy milk tomorrow @errands !` - Creates flagged task with context, defer date
- `Call mom next monday` - Parses "next monday" as defer date
- `Meeting #work-project due friday` - Adds to project with due date
- `2+2` or `(150/5)*3` - Built-in calculator
- Just start typing task names to search existing tasks

See `LAUNCHER.md` for complete documentation.

### GTD Workflow

1. **Capture**: Press Ctrl+Space or Ctrl+N and type tasks as they occur
2. **Clarify**: Add contexts (@home, @computer), set defer/due dates
3. **Organize**: Assign to projects, flag important items
4. **Review**: Use Review mode to check recently modified tasks
5. **Engage**: Check Today/Anytime perspectives, work on next actions

## Data Location

Your task database and preferences are stored at:

- **Linux**: `~/.local/share/samfocus/samfocus.db` and `preferences.txt`
- **Windows**: `%APPDATA%\samfocus\samfocus.db` and `preferences.txt`

Backups are created in the same directory with timestamps when using the Export feature.

## Project Structure

```
samfocus/
├── src/
│   ├── main.c                      # Application entry point
│   ├── core/
│   │   ├── platform.c/h            # Platform-specific utilities
│   │   ├── task.c/h                # Task data structures
│   │   ├── project.c/h             # Project management
│   │   ├── context.c/h             # Context/tag system
│   │   ├── undo.c/h                # Undo system
│   │   ├── export.c/h              # Export/backup functionality
│   │   └── preferences.c/h         # Preferences management
│   ├── db/
│   │   └── database.c/h            # SQLite database layer
│   ├── ui/
│   │   ├── inbox_view.c/h          # Main task view
│   │   ├── sidebar.c/h             # Navigation sidebar
│   │   ├── help_overlay.c/h        # Help system
│   │   ├── command_palette.c/h     # Command palette
│   │   ├── markdown.c/h            # Markdown renderer
│   │   └── launcher.c/h            # Quick launcher
│   └── cli/
│       └── samfocus-cli.c          # CLI companion tool
├── tests/
│   ├── test_framework.h            # Custom test framework
│   ├── unit/
│   │   └── test_database.c         # 26 unit tests
│   └── integration/
│       └── test_workflows.c        # 10 integration tests
├── external/
│   └── cimgui/                     # Dear ImGui C bindings (submodule)
├── .github/workflows/
│   └── ci.yml                      # GitHub Actions CI/CD
├── meson.build                     # Build configuration
├── README.md                       # This file
├── TESTING.md                      # Testing documentation
├── LAUNCHER.md                     # Launcher usage guide
└── LICENSE                         # MIT License
```

## Testing

Run all tests with:

```bash
./run_tests.sh          # Clean output
./run_tests.sh -v       # Verbose output
meson test -C build -v  # Direct meson call
```

**Test Coverage:**
- 26 unit tests (database operations)
- 10 integration tests (complete workflows)
- CI/CD with GitHub Actions
- 100% test pass rate

See `TESTING.md` for detailed testing documentation.

## Documentation

- **README.md** (this file) - Overview and usage
- **LAUNCHER.md** - Quick launcher guide with examples
- **TESTING.md** - Testing framework and test writing guide

## Development Philosophy

1. **Inbox first** - Capture should be frictionless
2. **Keyboard over mouse** - Speed and efficiency
3. **Local-first, offline always** - Your data, your machine
4. **No features unless they reduce friction** - Simplicity wins
5. **Ship something usable** - Incremental progress

## Contributing

This is a personal project, but contributions are welcome! Feel free to:
- Report bugs via GitHub Issues
- Suggest features
- Submit pull requests
- Fork and customize for your needs

## License

MIT License - see `LICENSE` file for details.

Copyright (c) 2025 SamFocus Contributors
