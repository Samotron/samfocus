#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>
#include <cimgui_impl.h>

#include "core/platform.h"
#include "core/task.h"
#include "core/project.h"
#include "core/context.h"
#include "core/undo.h"
#include "db/database.h"
#include "ui/inbox_view.h"
#include "ui/sidebar.h"
#include "ui/help_overlay.h"
#include "ui/command_palette.h"

// Forward declarations
static void glfw_error_callback(int error, const char* description);
static int init_imgui(GLFWwindow* window);
static void cleanup_imgui(void);

// Application state
static Task* tasks = NULL;
static int task_count = 0;
static Project* projects = NULL;
static int project_count = 0;
static Context* contexts = NULL;
static int context_count = 0;
static int selected_project_id = -1;  // -4 = Flagged, -3 = Anytime, -2 = Completed, -1 = Today, 0 = Inbox, >0 = Project ID
static int selected_context_id = 0;   // 0 = No filter, >0 = Filter by context
static CommandPaletteState cmd_palette;
static bool show_help_overlay = false;
static UndoStack undo_stack;

// Load tasks from database (optionally filtered by project)
static int load_tasks(int project_filter) {
    if (tasks != NULL) {
        free(tasks);
        tasks = NULL;
        task_count = 0;
    }
    
    // Load all incomplete tasks, we'll filter by project in the UI if needed
    if (db_load_tasks(&tasks, &task_count, -1) != 0) {
        fprintf(stderr, "Failed to load tasks: %s\n", db_get_error());
        return -1;
    }
    
    // Get current time for availability checking
    time_t now = time(NULL);
    struct tm* now_tm = localtime(&now);
    
    // Find project type if filtering by specific project
    ProjectType project_type = PROJECT_TYPE_SEQUENTIAL;
    if (project_filter > 0) {
        for (int j = 0; j < project_count; j++) {
            if (projects[j].id == project_filter) {
                project_type = projects[j].type;
                break;
            }
        }
    }
    
    int filtered_count = 0;
    for (int i = 0; i < task_count; i++) {
        // For Completed perspective, only show completed tasks
        if (project_filter == -2) {
            if (tasks[i].status == TASK_STATUS_DONE) {
                tasks[filtered_count++] = tasks[i];
            }
            continue;
        }
        
        // Skip deferred tasks (not yet available) for all other perspectives
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) {
            continue;
        }
        
        // Skip completed tasks for all perspectives except Completed
        if (tasks[i].status == TASK_STATUS_DONE) {
            continue;
        }
        
        if (project_filter == -4) {
            // Flagged perspective: show only flagged tasks (not deferred, not completed)
            if (tasks[i].flagged) {
                tasks[filtered_count++] = tasks[i];
            }
        } else if (project_filter == -3) {
            // Anytime perspective: show all available tasks (not deferred, not completed)
            tasks[filtered_count++] = tasks[i];
        } else if (project_filter == -1) {
            // Today perspective: show tasks that are due today or overdue, or have no due date but are available
            bool include = false;
            
            if (tasks[i].due_at > 0) {
                struct tm* due_tm = localtime(&tasks[i].due_at);
                // Check if due today or overdue
                if (due_tm->tm_year < now_tm->tm_year ||
                    (due_tm->tm_year == now_tm->tm_year && due_tm->tm_yday <= now_tm->tm_yday)) {
                    include = true;
                }
            } else {
                // Tasks without due date: include if they're available (not deferred)
                include = true;
            }
            
            if (include) {
                tasks[filtered_count++] = tasks[i];
            }
        } else if (project_filter == 0) {
            // Inbox: tasks with no project
            if (tasks[i].project_id == 0) {
                tasks[filtered_count++] = tasks[i];
            }
        } else {
            // Specific project: tasks assigned to this project
            if (tasks[i].project_id == project_filter) {
                if (project_type == PROJECT_TYPE_PARALLEL) {
                    // Parallel: show all tasks
                    tasks[filtered_count++] = tasks[i];
                } else {
                    // Sequential: only show first incomplete task
                    int first_task = db_get_first_incomplete_task_in_project(project_filter);
                    if (first_task == tasks[i].id) {
                        tasks[filtered_count++] = tasks[i];
                    }
                }
            }
        }
    }
    task_count = filtered_count;
    
    return 0;
}

// Load projects from database
static int load_projects(void) {
    if (projects != NULL) {
        free(projects);
        projects = NULL;
        project_count = 0;
    }
    
    if (db_load_projects(&projects, &project_count) != 0) {
        fprintf(stderr, "Failed to load projects: %s\n", db_get_error());
        return -1;
    }
    
    return 0;
}

// Load contexts from database
static int load_contexts(void) {
    if (contexts != NULL) {
        free(contexts);
        contexts = NULL;
        context_count = 0;
    }
    
    if (db_load_contexts(&contexts, &context_count) != 0) {
        fprintf(stderr, "Failed to load contexts: %s\n", db_get_error());
        return -1;
    }
    
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("SamFocus - Starting up...\n");
    
    // Get application data directory
    const char* app_dir = get_app_data_dir();
    printf("App data directory: %s\n", app_dir);
    
    // Ensure directory exists
    if (ensure_dir_exists(app_dir) != 0) {
        fprintf(stderr, "Failed to create app data directory\n");
        return 1;
    }
    
    // Initialize database
    const char* db_path = path_join(app_dir, "samfocus.db");
    printf("Database path: %s\n", db_path);
    
    if (db_init(db_path) != 0) {
        fprintf(stderr, "Failed to initialize database: %s\n", db_get_error());
        return 1;
    }
    
    if (db_create_schema() != 0) {
        fprintf(stderr, "Failed to create database schema: %s\n", db_get_error());
        db_close();
        return 1;
    }
    
    printf("Database initialized successfully\n");
    
    // Load projects, contexts, and tasks
    if (load_projects() != 0) {
        db_close();
        return 1;
    }
    
    if (load_contexts() != 0) {
        db_close();
        return 1;
    }
    
    if (load_tasks(selected_project_id) != 0) {
        db_close();
        return 1;
    }
    
    printf("Loaded %d projects, %d contexts, and %d tasks\n", project_count, context_count, task_count);
    
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        db_close();
        return 1;
    }
    
    // GL 3.3 + GLSL 330
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SamFocus", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        db_close();
        return 1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize ImGui
    if (init_imgui(window) != 0) {
        fprintf(stderr, "Failed to initialize ImGui\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        db_close();
        return 1;
    }
    
    printf("ImGui initialized successfully\n");
    
    // Initialize views
    sidebar_init();
    inbox_view_init();
    command_palette_init(&cmd_palette);
    undo_init(&undo_stack);
    
    printf("Entering main loop...\n");
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        
        // Get window dimensions for layout
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        const float sidebar_width = 250.0f;
        
        // Check if '?' key is pressed (Shift + /) to toggle help
        ImGuiIO* io = igGetIO_Nil();
        if (io->KeyShift && igIsKeyPressed_Bool(ImGuiKey_Slash, false)) {
            show_help_overlay = !show_help_overlay;
        }
        
        // Escape to close help overlay
        if (show_help_overlay && igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
            show_help_overlay = false;
        }
        
        // Ctrl+K to open command palette
        if (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_K, false)) {
            command_palette_open(&cmd_palette);
            igOpenPopup_Str("Command Palette", ImGuiPopupFlags_None);
        }
        
        // Ctrl+1-5 for perspective switching
        if (io->KeyCtrl) {
            // Ctrl+Z for undo
            if (igIsKeyPressed_Bool(ImGuiKey_Z, false)) {
                if (undo_can_undo(&undo_stack)) {
                    undo_last(&undo_stack);
                    load_tasks(selected_project_id);
                    load_projects();
                }
            }
            
            if (igIsKeyPressed_Bool(ImGuiKey_1, false)) {
                selected_project_id = -1;  // Today
            } else if (igIsKeyPressed_Bool(ImGuiKey_2, false)) {
                selected_project_id = -3;  // Anytime
            } else if (igIsKeyPressed_Bool(ImGuiKey_3, false)) {
                selected_project_id = -4;  // Flagged
            } else if (igIsKeyPressed_Bool(ImGuiKey_4, false)) {
                selected_project_id = 0;   // Inbox
            } else if (igIsKeyPressed_Bool(ImGuiKey_5, false)) {
                selected_project_id = -2;  // Completed
            }
        }
        
        // Position and render sidebar on the left
        igSetNextWindowPos((ImVec2){0, 0}, ImGuiCond_Always, (ImVec2){0, 0});
        igSetNextWindowSize((ImVec2){sidebar_width, (float)display_h}, ImGuiCond_Always);
        int sidebar_needs_reload = 0;
        int prev_project = selected_project_id;
        int prev_context = selected_context_id;
        sidebar_render(projects, project_count, contexts, context_count, 
                      tasks, task_count,
                      &selected_project_id, &selected_context_id, &sidebar_needs_reload);
        
        // If project or context changed, reload tasks
        if (prev_project != selected_project_id || prev_context != selected_context_id) {
            load_tasks(selected_project_id);
        }
        
        // Position and render inbox/project view on the right
        igSetNextWindowPos((ImVec2){sidebar_width, 0}, ImGuiCond_Always, (ImVec2){0, 0});
        igSetNextWindowSize((ImVec2){(float)display_w - sidebar_width, (float)display_h}, ImGuiCond_Always);
        int inbox_needs_reload = 0;
        inbox_view_render(tasks, task_count, projects, project_count, 
                         contexts, context_count, selected_project_id, &inbox_needs_reload);
        
        // Reload if needed
        if (sidebar_needs_reload) {
            load_projects();
            load_contexts();
        }
        if (inbox_needs_reload) {
            load_tasks(selected_project_id);
        }
        
        // Render command palette
        if (command_palette_show(&cmd_palette, tasks, task_count, projects, project_count,
                                contexts, context_count, -1)) {
            // Handle command palette result
            CommandResult* selected = &cmd_palette.results[cmd_palette.selected_index];
            if (selected->type == CMD_TYPE_PROJECT) {
                selected_project_id = selected->id;
                load_tasks(selected_project_id);
            } else if (selected->type == CMD_TYPE_CONTEXT) {
                selected_context_id = selected->id;
                load_tasks(selected_project_id);
            } else if (selected->type == CMD_TYPE_TASK) {
                // Jump to the task's project
                for (int i = 0; i < task_count; i++) {
                    if (tasks[i].id == selected->id) {
                        if (tasks[i].project_id > 0) {
                            selected_project_id = tasks[i].project_id;
                        } else {
                            selected_project_id = 0;  // Inbox
                        }
                        load_tasks(selected_project_id);
                        break;
                    }
                }
            }
            // Actions would require getting the current selected task ID from inbox_view
            // For now, actions in command palette are documented but not fully implemented
        }
        
        // Render help overlay if toggled
        help_overlay_render(show_help_overlay);
        
        // Rendering
        igRender();
        
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    printf("Shutting down...\n");
    
    // Cleanup
    sidebar_cleanup();
    inbox_view_cleanup();
    
    if (tasks != NULL) {
        free(tasks);
    }
    
    if (projects != NULL) {
        free(projects);
    }
    
    if (contexts != NULL) {
        free(contexts);
    }
    
    cleanup_imgui();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    db_close();
    
    printf("Goodbye!\n");
    return 0;
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static int init_imgui(GLFWwindow* window) {
    // Setup Dear ImGui context
    igCreateContext(NULL);
    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup Platform/Renderer backends
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        return -1;
    }
    
    const char* glsl_version = "#version 330";
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        ImGui_ImplGlfw_Shutdown();
        return -1;
    }
    
    // Setup modern dark style
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;
    
    // Modern dark theme inspired by VS Code, Notion, Linear
    // Background colors - deep dark grays
    colors[ImGuiCol_WindowBg]           = (ImVec4){0.11f, 0.11f, 0.13f, 1.00f};
    colors[ImGuiCol_ChildBg]            = (ImVec4){0.13f, 0.13f, 0.15f, 1.00f};
    colors[ImGuiCol_PopupBg]            = (ImVec4){0.09f, 0.09f, 0.11f, 0.98f};
    colors[ImGuiCol_Border]             = (ImVec4){0.22f, 0.22f, 0.26f, 1.00f};
    colors[ImGuiCol_BorderShadow]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    
    // Frame backgrounds (inputs, selectable items)
    colors[ImGuiCol_FrameBg]            = (ImVec4){0.16f, 0.16f, 0.19f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]     = (ImVec4){0.20f, 0.20f, 0.24f, 1.00f};
    colors[ImGuiCol_FrameBgActive]      = (ImVec4){0.24f, 0.24f, 0.29f, 1.00f};
    
    // Title bar
    colors[ImGuiCol_TitleBg]            = (ImVec4){0.09f, 0.09f, 0.11f, 1.00f};
    colors[ImGuiCol_TitleBgActive]      = (ImVec4){0.11f, 0.11f, 0.13f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]   = (ImVec4){0.09f, 0.09f, 0.11f, 0.75f};
    
    // Menu bar
    colors[ImGuiCol_MenuBarBg]          = (ImVec4){0.11f, 0.11f, 0.13f, 1.00f};
    
    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]        = (ImVec4){0.11f, 0.11f, 0.13f, 0.53f};
    colors[ImGuiCol_ScrollbarGrab]      = (ImVec4){0.26f, 0.26f, 0.31f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered] = (ImVec4){0.32f, 0.32f, 0.38f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive] = (ImVec4){0.38f, 0.38f, 0.45f, 1.00f};
    
    // Checkboxes and sliders
    colors[ImGuiCol_CheckMark]          = (ImVec4){0.53f, 0.70f, 1.00f, 1.00f};
    colors[ImGuiCol_SliderGrab]         = (ImVec4){0.53f, 0.70f, 1.00f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]   = (ImVec4){0.63f, 0.78f, 1.00f, 1.00f};
    
    // Buttons
    colors[ImGuiCol_Button]             = (ImVec4){0.26f, 0.26f, 0.31f, 1.00f};
    colors[ImGuiCol_ButtonHovered]      = (ImVec4){0.32f, 0.32f, 0.38f, 1.00f};
    colors[ImGuiCol_ButtonActive]       = (ImVec4){0.38f, 0.38f, 0.45f, 1.00f};
    
    // Headers (collapsing, trees)
    colors[ImGuiCol_Header]             = (ImVec4){0.22f, 0.22f, 0.26f, 1.00f};
    colors[ImGuiCol_HeaderHovered]      = (ImVec4){0.26f, 0.26f, 0.31f, 1.00f};
    colors[ImGuiCol_HeaderActive]       = (ImVec4){0.30f, 0.30f, 0.36f, 1.00f};
    
    // Separators
    colors[ImGuiCol_Separator]          = (ImVec4){0.22f, 0.22f, 0.26f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]   = (ImVec4){0.32f, 0.32f, 0.38f, 1.00f};
    colors[ImGuiCol_SeparatorActive]    = (ImVec4){0.42f, 0.42f, 0.50f, 1.00f};
    
    // Resize grip
    colors[ImGuiCol_ResizeGrip]         = (ImVec4){0.26f, 0.26f, 0.31f, 0.50f};
    colors[ImGuiCol_ResizeGripHovered]  = (ImVec4){0.32f, 0.32f, 0.38f, 0.75f};
    colors[ImGuiCol_ResizeGripActive]   = (ImVec4){0.38f, 0.38f, 0.45f, 1.00f};
    
    // Tabs
    colors[ImGuiCol_Tab]                = (ImVec4){0.16f, 0.16f, 0.19f, 1.00f};
    colors[ImGuiCol_TabHovered]         = (ImVec4){0.26f, 0.26f, 0.31f, 1.00f};
    colors[ImGuiCol_TabSelected]        = (ImVec4){0.22f, 0.22f, 0.26f, 1.00f};
    colors[ImGuiCol_TabDimmed]          = (ImVec4){0.13f, 0.13f, 0.15f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]  = (ImVec4){0.18f, 0.18f, 0.21f, 1.00f};
    
    // Table colors
    colors[ImGuiCol_TableHeaderBg]      = (ImVec4){0.16f, 0.16f, 0.19f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]  = (ImVec4){0.26f, 0.26f, 0.31f, 1.00f};
    colors[ImGuiCol_TableBorderLight]   = (ImVec4){0.22f, 0.22f, 0.26f, 1.00f};
    colors[ImGuiCol_TableRowBg]         = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]      = (ImVec4){1.00f, 1.00f, 1.00f, 0.03f};
    
    // Text selection
    colors[ImGuiCol_TextSelectedBg]     = (ImVec4){0.53f, 0.70f, 1.00f, 0.35f};
    
    // Drag drop
    colors[ImGuiCol_DragDropTarget]     = (ImVec4){0.53f, 0.70f, 1.00f, 0.90f};
    
    // Nav cursor
    colors[ImGuiCol_NavCursor]          = (ImVec4){0.53f, 0.70f, 1.00f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]  = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
    
    // Modal window dim
    colors[ImGuiCol_ModalWindowDimBg]   = (ImVec4){0.00f, 0.00f, 0.00f, 0.60f};
    
    // Text colors
    colors[ImGuiCol_Text]               = (ImVec4){0.95f, 0.95f, 0.97f, 1.00f};
    colors[ImGuiCol_TextDisabled]       = (ImVec4){0.50f, 0.50f, 0.54f, 1.00f};
    
    // Style adjustments for modern look
    style->WindowPadding    = (ImVec2){12.0f, 12.0f};
    style->FramePadding     = (ImVec2){8.0f, 5.0f};
    style->CellPadding      = (ImVec2){8.0f, 4.0f};
    style->ItemSpacing      = (ImVec2){10.0f, 6.0f};
    style->ItemInnerSpacing = (ImVec2){8.0f, 6.0f};
    style->TouchExtraPadding = (ImVec2){0.0f, 0.0f};
    style->IndentSpacing    = 20.0f;
    style->ScrollbarSize    = 14.0f;
    style->GrabMinSize      = 12.0f;
    
    // Border and rounding
    style->WindowBorderSize = 1.0f;
    style->ChildBorderSize  = 1.0f;
    style->PopupBorderSize  = 1.0f;
    style->FrameBorderSize  = 0.0f;
    style->TabBorderSize    = 0.0f;
    
    // Rounding for modern look
    style->WindowRounding    = 6.0f;
    style->ChildRounding     = 4.0f;
    style->FrameRounding     = 4.0f;
    style->PopupRounding     = 6.0f;
    style->ScrollbarRounding = 8.0f;
    style->GrabRounding      = 4.0f;
    style->TabRounding       = 4.0f;
    
    // Other style
    style->WindowTitleAlign = (ImVec2){0.5f, 0.5f};
    style->ButtonTextAlign  = (ImVec2){0.5f, 0.5f};
    style->SelectableTextAlign = (ImVec2){0.0f, 0.5f};
    
    return 0;
}

static void cleanup_imgui(void) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);
}
