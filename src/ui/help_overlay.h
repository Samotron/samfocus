#ifndef HELP_OVERLAY_H
#define HELP_OVERLAY_H

#include <stdbool.h>

/**
 * Render the help overlay if it should be visible.
 * Call this every frame after all other UI.
 * 
 * @param show_help true if '?' key is being held
 */
void help_overlay_render(bool show_help);

#endif // HELP_OVERLAY_H
