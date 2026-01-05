#include "markdown.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Check if line starts with a pattern
static bool starts_with(const char* line, const char* pattern) {
    return strncmp(line, pattern, strlen(pattern)) == 0;
}

// Render a line of text with inline formatting
static void render_inline(const char* text) {
    char buffer[2048];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* pos = buffer;
    
    while (*pos) {
        // Check for bold **text** - just show in brighter color
        if (pos[0] == '*' && pos[1] == '*') {
            // Find closing **
            char* end = strstr(pos + 2, "**");
            if (end) {
                *end = '\0';
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 1.0f, 1.0f, 1.0f});
                igText("%s", pos + 2);
                igPopStyleColor(1);
                pos = end + 2;
                if (*pos) {
                    igSameLine(0, 0);
                }
                continue;
            }
        }
        
        // Check for italic *text*
        if (pos[0] == '*' && pos[1] != '*') {
            // Find closing *
            char* end = strchr(pos + 1, '*');
            if (end) {
                *end = '\0';
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.7f, 0.9f, 1.0f, 1.0f});
                igText("%s", pos + 1);
                igPopStyleColor(1);
                pos = end + 1;
                if (*pos) {
                    igSameLine(0, 0);
                }
                continue;
            }
        }
        
        // Check for inline code `code`
        if (pos[0] == '`') {
            // Find closing `
            char* end = strchr(pos + 1, '`');
            if (end) {
                *end = '\0';
                igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 0.9f, 0.7f, 1.0f});
                igText("%s", pos + 1);
                igPopStyleColor(1);
                pos = end + 1;
                if (*pos) {
                    igSameLine(0, 0);
                }
                continue;
            }
        }
        
        // Regular text - find next special char or end
        char* next_special = pos;
        while (*next_special && *next_special != '*' && *next_special != '`') {
            next_special++;
        }
        
        if (next_special > pos) {
            char saved = *next_special;
            *next_special = '\0';
            igText("%s", pos);
            *next_special = saved;
            pos = next_special;
            if (*pos) {
                igSameLine(0, 0);
            }
        } else {
            pos++;
        }
    }
    
    igText("");  // New line
}

void markdown_render(const char* text) {
    if (!text || text[0] == '\0') {
        igTextDisabled("No notes");
        return;
    }
    
    char buffer[4096];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* line = strtok(buffer, "\n");
    
    while (line != NULL) {
        // Trim leading whitespace
        while (*line && isspace(*line)) line++;
        
        if (line[0] == '\0') {
            // Empty line
            igSpacing();
        } else if (starts_with(line, "### ")) {
            // H3 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.7f, 1.0f, 0.7f, 1.0f});
            igText("%s", line + 4);
            igPopStyleColor(1);
            igSeparator();
        } else if (starts_with(line, "## ")) {
            // H2 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.8f, 1.0f, 0.8f, 1.0f});
            igText("%s", line + 3);
            igPopStyleColor(1);
            igSeparator();
        } else if (starts_with(line, "# ")) {
            // H1 header
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){1.0f, 1.0f, 0.7f, 1.0f});
            igText("%s", line + 2);
            igPopStyleColor(1);
            igSeparator();
        } else if (starts_with(line, "- ") || starts_with(line, "* ")) {
            // Bullet list
            igBullet();
            igSameLine(0, 0);
            render_inline(line + 2);
        } else if (isdigit(line[0]) && line[1] == '.' && line[2] == ' ') {
            // Numbered list
            char num[2] = {line[0], '\0'};
            igText("%s.", num);
            igSameLine(0, 5);
            render_inline(line + 3);
        } else if (starts_with(line, "```")) {
            // Code block start/end (simplified - just show as monospace)
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4){0.8f, 0.8f, 0.8f, 1.0f});
            igText("---");
            igPopStyleColor(1);
        } else {
            // Regular text with inline formatting
            render_inline(line);
        }
        
        line = strtok(NULL, "\n");
    }
}
