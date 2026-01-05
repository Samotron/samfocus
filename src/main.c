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
#include "db/database.h"
#include "ui/inbox_view.h"

// Forward declarations
static void glfw_error_callback(int error, const char* description);
static int init_imgui(GLFWwindow* window);
static void cleanup_imgui(void);

// Application state
static Task* tasks = NULL;
static int task_count = 0;

// Load tasks from database
static int load_tasks(void) {
    if (tasks != NULL) {
        free(tasks);
        tasks = NULL;
        task_count = 0;
    }
    
    // Load inbox tasks
    if (db_load_tasks(&tasks, &task_count, TASK_STATUS_INBOX) != 0) {
        fprintf(stderr, "Failed to load tasks: %s\n", db_get_error());
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
    
    // Load tasks
    if (load_tasks() != 0) {
        db_close();
        return 1;
    }
    
    printf("Loaded %d tasks\n", task_count);
    
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
    
    // Initialize inbox view
    inbox_view_init();
    
    printf("Entering main loop...\n");
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        igNewFrame();
        
        // Render UI
        int needs_reload = 0;
        inbox_view_render(tasks, task_count, &needs_reload);
        
        // Reload tasks if needed
        if (needs_reload) {
            load_tasks();
        }
        
        // Rendering
        igRender();
        
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    printf("Shutting down...\n");
    
    // Cleanup
    inbox_view_cleanup();
    
    if (tasks != NULL) {
        free(tasks);
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
