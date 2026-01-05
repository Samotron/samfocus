# SamFocus

A personal OmniFocus-style task management application built with C, Dear ImGui, and SQLite.

## Features (Phase 0-3 Complete)

- **Project Management**: Organize tasks into sequential projects
- **Sidebar Navigation**: Quick access to Inbox and all projects
- **Sequential Projects**: Automatically show only the next available action
- **Task Assignment**: Easily move tasks between projects via dropdown
- **Inbox view**: Capture tasks quickly with zero friction
- **Full keyboard navigation**: Mouse optional for all operations
- **Arrow key navigation**: Move through tasks with Up/Down
- **Quick actions**: Space to complete, Delete to remove, Enter to edit
- **Persistent storage**: SQLite database with platform-specific data directories
- **Cross-platform**: Works on Linux and Windows
- **Local-first**: No sync, no cloud, your data stays with you

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
3. Install [Visual Studio](https://visualstudio.microsoft.com/) with C++ tools
4. Dependencies (GLFW, SQLite) will be handled by meson automatically

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
./build/samfocus
```

### Development Build

For faster iteration during development:

```bash
meson setup build --buildtype=debug
meson compile -C build && ./build/samfocus
```

## Usage

### Keyboard Shortcuts

**Adding Tasks:**
- **Ctrl+N**: Focus new task input (works from anywhere)
- **Enter**: Add new task (when in input field)

**Navigating Tasks:**
- **Up/Down Arrow**: Navigate through task list
- **Home**: Jump to first task
- **End**: Jump to last task

**Task Actions:**
- **Enter**: Edit selected task
- **Space**: Toggle completion status of selected task
- **Ctrl+Enter**: Toggle completion status of selected task
- **Delete**: Remove selected task
- **Escape**: Cancel edit mode

**Mouse Actions:**
- **Click task text**: Select and edit task
- **Right-click task**: Select task (without editing)
- **Checkbox**: Toggle completion status
- **Delete button**: Remove task
- **Project dropdown**: Assign task to a project or move to Inbox

**Projects:**
- **Click "Inbox"**: View all unassigned tasks
- **Click project name**: View tasks in that project (sequential projects show only next action)
- **Click "+"**: Create a new project
- **Right-click project**: Rename or delete
- **→ symbol**: Sequential project (shows only next action)

### GTD Workflow

1. **Capture**: Press Ctrl+N anywhere and type a task
2. **Organize**: Use the project dropdown to assign tasks
3. **Review**: Click through projects in the sidebar
4. **Do**: Sequential projects automatically show only the next action

## Data Location

Your task database is stored at:

- **Linux**: `~/.local/share/samfocus/samfocus.db`
- **Windows**: `%APPDATA%\samfocus\samfocus.db`

## Project Structure

```
samfocus/
├── src/
│   ├── main.c              # Application entry point
│   ├── core/
│   │   ├── platform.c/h    # Platform-specific utilities
│   │   ├── task.c/h        # Task data structures
│   │   └── project.c/h     # Project data structures
│   ├── db/
│   │   └── database.c/h    # SQLite database layer
│   └── ui/
│       ├── inbox_view.c/h  # Task list UI
│       └── sidebar.c/h     # Project sidebar UI
├── external/
│   └── cimgui/             # Dear ImGui C bindings (submodule)
└── meson.build             # Build configuration
```

## Roadmap

This project follows an 8-phase development plan:

- [x] **Phase 0**: Ground rules & setup
- [x] **Phase 1**: Skeleton & persistence
- [x] **Phase 2**: Inbox + keyboard flow
- [x] **Phase 3**: Projects (Sequential only)
- [ ] **Phase 4**: Defer & due dates
- [ ] **Phase 5**: Tags/contexts
- [ ] **Phase 6**: Review mode
- [ ] **Phase 7**: Perspectives
- [ ] **Phase 8**: Polish

See `PLAN.md` for detailed phase breakdown.

## Development Philosophy

From the original plan:

1. **Inbox first**
2. **Keyboard over mouse**
3. **Local-first, offline always**
4. **No features unless they reduce friction**
5. **Ship something usable every weekend**

## License

Personal project - use as you wish.
