# SamFocus

A personal OmniFocus-style task management application built with C, Dear ImGui, and SQLite.

## Features (Phase 0 + 1)

- **Inbox view**: Capture tasks quickly
- **Persistent storage**: SQLite database with platform-specific data directories
- **Cross-platform**: Works on Linux and Windows
- **Keyboard-driven**: Minimal mouse required
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

- **Ctrl+N**: Focus new task input (from anywhere)
- **Enter**: Add new task or save edit
- **Escape**: Cancel current input/edit
- **Click task text**: Edit task
- **Checkbox**: Mark task as complete/incomplete
- **Delete button**: Remove task

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
│   │   └── task.c/h        # Task data structures
│   ├── db/
│   │   └── database.c/h    # SQLite database layer
│   └── ui/
│       └── inbox_view.c/h  # Inbox UI
├── external/
│   └── cimgui/             # Dear ImGui C bindings (submodule)
└── meson.build             # Build configuration
```

## Roadmap

This project follows an 8-phase development plan:

- [x] **Phase 0**: Ground rules & setup
- [x] **Phase 1**: Skeleton & persistence
- [ ] **Phase 2**: Inbox + keyboard flow
- [ ] **Phase 3**: Projects
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
