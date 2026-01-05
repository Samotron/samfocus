#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <stdbool.h>
#include <stddef.h>

// Launcher hotkey configuration
typedef struct {
    int key;  // ImGuiKey value
    bool ctrl;
    bool alt;
    bool shift;
    bool super;  // Windows/Command key
} HotkeyConfig;

// Application preferences
typedef struct {
    HotkeyConfig launcher_hotkey;
    bool calculator_enabled;
    bool clipboard_history_enabled;
    bool system_commands_enabled;
    int clipboard_history_size;
    char theme[64];
} Preferences;

/**
 * Initialize preferences with defaults
 */
void preferences_init(Preferences* prefs);

/**
 * Load preferences from file
 */
int preferences_load(Preferences* prefs);

/**
 * Save preferences to file
 */
int preferences_save(const Preferences* prefs);

/**
 * Render preferences window
 */
void preferences_render(Preferences* prefs, bool* show_preferences);

/**
 * Check if a hotkey matches current input
 */
bool preferences_hotkey_pressed(const HotkeyConfig* hotkey);

/**
 * Get hotkey display string
 */
void preferences_hotkey_string(const HotkeyConfig* hotkey, char* buffer, size_t size);

#endif // PREFERENCES_H
