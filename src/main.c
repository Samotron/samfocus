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
#include "db/database.h"
#include "ui/inbox_view.h"
#include "ui/sidebar.h"
#include "ui/help_overlay.h"

// Forward declarations
static void glfw_error_callback(int error, const char* description);
static int init_imgui(GLFWwindow* window);
static void cleanup_imgui(void);

// Application state
static Task* tasks = NULL;
static int task_count = 0;
static Project* projects = NULL;
static int project_count = 0;
static int selected_project_id = -1;  // -1 = Today, 0 = Inbox, >0 = Project ID

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
        // Skip deferred tasks (not yet available) for all perspectives
        if (tasks[i].defer_at > 0 && tasks[i].defer_at > now) {
            continue;
        }
        
        // Skip completed tasks
        if (tasks[i].status == TASK_STATUS_DONE) {
            continue;
        }
        
        if (project_filter == -1) {
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
    
    // Load projects and tasks
    if (load_projects() != 0) {
        db_close();
        return 1;
    }
    
    if (load_tasks(selected_project_id) != 0) {
        db_close();
        return 1;
    }
    
    printf("Loaded %d projects and %d tasks\n", project_count, task_count);
    
    // Setup GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        db_close();
        return 1;
    }
    
    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
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
        
        // Check if '?' key is held (Shift + /)
        ImGuiIO* io = igGetIO_Nil();
        bool show_help = io->KeyShift && igIsKeyDown_Nil(ImGuiKey_Slash);
        
        // Position and render sidebar on the left
        igSetNextWindowPos((ImVec2){0, 0}, ImGuiCond_Always, (ImVec2){0, 0});
        igSetNextWindowSize((ImVec2){sidebar_width, (float)display_h}, ImGuiCond_Always);
        int sidebar_needs_reload = 0;
        int prev_project = selected_project_id;
        sidebar_render(projects, project_count, &selected_project_id, &sidebar_needs_reload);
        
        // If project changed, reload tasks
        if (prev_project != selected_project_id) {
            load_tasks(selected_project_id);
        }
        
        // Position and render inbox/project view on the right
        igSetNextWindowPos((ImVec2){sidebar_width, 0}, ImGuiCond_Always, (ImVec2){0, 0});
        igSetNextWindowSize((ImVec2){(float)display_w - sidebar_width, (float)display_h}, ImGuiCond_Always);
        int inbox_needs_reload = 0;
        inbox_view_render(tasks, task_count, projects, project_count, 
                         selected_project_id, &inbox_needs_reload);
        
        // Reload if needed
        if (sidebar_needs_reload) {
            load_projects();
        }
        if (inbox_needs_reload) {
            load_tasks(selected_project_id);
        }
        
        // Render help overlay if '?' is held
        help_overlay_render(show_help);
        
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
    
    // Setup style
    igStyleColorsDark(NULL);
    
    return 0;
}

static void cleanup_imgui(void) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);
}
