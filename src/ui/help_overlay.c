#include "help_overlay.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui.h>

void help_overlay_render(bool show_help) {
    if (!show_help) {
        return;
    }
    
    // Get main viewport size
    ImGuiViewport* viewport = igGetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;
    
    // Center the help window
    ImVec2 center = {work_pos.x + work_size.x * 0.5f, work_pos.y + work_size.y * 0.5f};
    igSetNextWindowPos(center, ImGuiCond_Always, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){600, 500}, ImGuiCond_Always);
    
    // Semi-transparent background
    igSetNextWindowBgAlpha(0.95f);
    
    int flags = ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings;
    
    if (igBegin("Keyboard Shortcuts (Hold ? to view)", NULL, flags)) {
        igTextColored((ImVec4){0.4f, 0.8f, 1.0f, 1.0f}, "SamFocus - Keyboard Shortcuts");
        igSeparator();
        igSpacing();
        
        // Adding Tasks section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Adding Tasks:");
        igBulletText("Ctrl+N - Focus new task input (works anywhere)");
        igBulletText("Enter - Add new task (when in input field)");
        igSpacing();
        
        // Navigation section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Navigation:");
        igBulletText("Up/Down Arrow - Navigate through task list");
        igBulletText("Home - Jump to first task");
        igBulletText("End - Jump to last task");
        igSpacing();
        
        // Task Actions section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Task Actions:");
        igBulletText("Enter - Edit selected task");
        igBulletText("Space - Toggle completion status");
        igBulletText("Ctrl+Enter - Toggle completion status");
        igBulletText("Delete - Remove selected task");
        igBulletText("Escape - Cancel edit mode");
        igBulletText("Star (☆/★) button - Flag/unflag task");
        igBulletText("↑ button - Move task up in list");
        igBulletText("↓ button - Move task down in list");
        igSpacing();
        
        // Projects section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Perspectives & Projects:");
        igBulletText("'Today' - Tasks due today/overdue + available tasks");
        igBulletText("'Anytime' - All available tasks (not deferred)");
        igBulletText("'Flagged' - Starred/important tasks");
        igBulletText("'Inbox' - Unassigned tasks");
        igBulletText("'Completed' - Finished tasks");
        igBulletText("Click project - View project tasks");
        igBulletText("Click '+' - Create new project");
        igBulletText("Right-click project - Rename, delete, or toggle type");
        igBulletText("Use dropdown (→) - Assign task to project");
        igSpacing();
        
        // Dates section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Defer & Due Dates:");
        igBulletText("'Defer' button - Hide task until tomorrow");
        igBulletText("'Due' button - Set due date to tomorrow");
        igBulletText("'X' button - Clear defer or due date");
        igBulletText("Red date - Overdue task");
        igBulletText("Yellow date - Due today");
        igBulletText("Green date - Due in the future");
        igSpacing();
        
        // Mouse Actions section
        igTextColored((ImVec4){1.0f, 0.8f, 0.2f, 1.0f}, "Mouse Actions:");
        igBulletText("Click task text - Select and edit");
        igBulletText("Right-click task - Select without editing");
        igBulletText("Checkbox - Toggle completion");
        igSpacing();
        
        igSeparator();
        igSpacing();
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, 
                     "Sequential Projects (→) show only the next action");
        igTextColored((ImVec4){0.7f, 0.7f, 0.7f, 1.0f}, 
                     "Complete a task to reveal the next one!");
    }
    igEnd();
}
