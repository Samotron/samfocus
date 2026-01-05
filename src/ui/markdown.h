#ifndef MARKDOWN_H
#define MARKDOWN_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>

/**
 * Render markdown text in ImGui.
 * Supports:
 * - # Headers (H1-H3)
 * - **bold** and *italic*
 * - `inline code`
 * - - bullet lists
 * - 1. numbered lists
 * 
 * @param text Markdown text to render
 */
void markdown_render(const char* text);

#endif // MARKDOWN_H
