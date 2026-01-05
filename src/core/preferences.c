#include "preferences.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Platform-specific config path
static void get_config_path(char* path, size_t size) {
#ifdef _WIN32
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(path, size, "%s/samfocus/preferences.txt", appdata);
    } else {
        snprintf(path, size, "preferences.txt");
    }
#else
    const char* home = getenv("HOME");
    if (home) {
        snprintf(path, size, "%s/.local/share/samfocus/preferences.txt", home);
    } else {
        snprintf(path, size, "preferences.txt");
    }
#endif
}

void preferences_init(Preferences* prefs) {
    // Default launcher hotkey: Ctrl+Space
    prefs->launcher_hotkey.key = ImGuiKey_Space;
    prefs->launcher_hotkey.ctrl = true;
    prefs->launcher_hotkey.alt = false;
    prefs->launcher_hotkey.shift = false;
    prefs->launcher_hotkey.super = false;
    
    // Enable Raycast features by default
    prefs->calculator_enabled = true;
    prefs->clipboard_history_enabled = true;
    prefs->system_commands_enabled = true;
    prefs->clipboard_history_size = 50;
    
    strncpy(prefs->theme, "default", sizeof(prefs->theme) - 1);
}

int preferences_load(Preferences* prefs) {
    char config_path[512];
    get_config_path(config_path, sizeof(config_path));
    
    FILE* f = fopen(config_path, "r");
    if (!f) {
        // File doesn't exist, use defaults
        preferences_init(prefs);
        return 0;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char key[64], value[192];
        if (sscanf(line, "%63[^=]=%191s", key, value) == 2) {
            if (strcmp(key, "launcher_key") == 0) {
                prefs->launcher_hotkey.key = atoi(value);
            } else if (strcmp(key, "launcher_ctrl") == 0) {
                prefs->launcher_hotkey.ctrl = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "launcher_alt") == 0) {
                prefs->launcher_hotkey.alt = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "launcher_shift") == 0) {
                prefs->launcher_hotkey.shift = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "launcher_super") == 0) {
                prefs->launcher_hotkey.super = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "calculator_enabled") == 0) {
                prefs->calculator_enabled = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "clipboard_history_enabled") == 0) {
                prefs->clipboard_history_enabled = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "system_commands_enabled") == 0) {
                prefs->system_commands_enabled = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "clipboard_history_size") == 0) {
                prefs->clipboard_history_size = atoi(value);
            } else if (strcmp(key, "theme") == 0) {
                strncpy(prefs->theme, value, sizeof(prefs->theme) - 1);
            }
        }
    }
    
    fclose(f);
    return 0;
}

int preferences_save(const Preferences* prefs) {
    char config_path[512];
    get_config_path(config_path, sizeof(config_path));
    
    FILE* f = fopen(config_path, "w");
    if (!f) {
        return -1;
    }
    
    fprintf(f, "launcher_key=%d\n", prefs->launcher_hotkey.key);
    fprintf(f, "launcher_ctrl=%s\n", prefs->launcher_hotkey.ctrl ? "true" : "false");
    fprintf(f, "launcher_alt=%s\n", prefs->launcher_hotkey.alt ? "true" : "false");
    fprintf(f, "launcher_shift=%s\n", prefs->launcher_hotkey.shift ? "true" : "false");
    fprintf(f, "launcher_super=%s\n", prefs->launcher_hotkey.super ? "true" : "false");
    fprintf(f, "calculator_enabled=%s\n", prefs->calculator_enabled ? "true" : "false");
    fprintf(f, "clipboard_history_enabled=%s\n", prefs->clipboard_history_enabled ? "true" : "false");
    fprintf(f, "system_commands_enabled=%s\n", prefs->system_commands_enabled ? "true" : "false");
    fprintf(f, "clipboard_history_size=%d\n", prefs->clipboard_history_size);
    fprintf(f, "theme=%s\n", prefs->theme);
    
    fclose(f);
    return 0;
}

bool preferences_hotkey_pressed(const HotkeyConfig* hotkey) {
    ImGuiIO* io = igGetIO_Nil();
    
    bool modifiers_match = true;
    if (hotkey->ctrl != io->KeyCtrl) modifiers_match = false;
    if (hotkey->alt != io->KeyAlt) modifiers_match = false;
    if (hotkey->shift != io->KeyShift) modifiers_match = false;
    if (hotkey->super != io->KeySuper) modifiers_match = false;
    
    return modifiers_match && igIsKeyPressed_Bool(hotkey->key, false);
}

void preferences_hotkey_string(const HotkeyConfig* hotkey, char* buffer, size_t size) {
    buffer[0] = '\0';
    
    if (hotkey->ctrl) strncat(buffer, "Ctrl+", size - strlen(buffer) - 1);
    if (hotkey->alt) strncat(buffer, "Alt+", size - strlen(buffer) - 1);
    if (hotkey->shift) strncat(buffer, "Shift+", size - strlen(buffer) - 1);
    if (hotkey->super) strncat(buffer, "Super+", size - strlen(buffer) - 1);
    
    // Convert key enum to string
    const char* key_name = igGetKeyName(hotkey->key);
    if (key_name) {
        strncat(buffer, key_name, size - strlen(buffer) - 1);
    }
}

void preferences_render(Preferences* prefs, bool* show_preferences) {
    if (!*show_preferences) return;
    
    ImGuiIO* io = igGetIO_Nil();
    ImVec2 center = {io->DisplaySize.x * 0.5f, io->DisplaySize.y * 0.5f};
    igSetNextWindowPos(center, ImGuiCond_Appearing, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){500, 400}, ImGuiCond_Appearing);
    
    if (igBegin("Preferences", show_preferences, ImGuiWindowFlags_None)) {
        igTextColored((ImVec4){0.4f, 0.8f, 1.0f, 1.0f}, "SamFocus Preferences");
        igSeparator();
        igSpacing();
        
        // Launcher Hotkey Section
        if (igCollapsingHeader_TreeNodeFlags("Launcher Hotkey", ImGuiTreeNodeFlags_DefaultOpen)) {
            igText("Customize the global hotkey for Quick Launcher");
            igSpacing();
            
            igCheckbox("Ctrl", &prefs->launcher_hotkey.ctrl);
            igSameLine(0, 10);
            igCheckbox("Alt", &prefs->launcher_hotkey.alt);
            igSameLine(0, 10);
            igCheckbox("Shift", &prefs->launcher_hotkey.shift);
            igSameLine(0, 10);
            igCheckbox("Super/Win", &prefs->launcher_hotkey.super);
            
            igSpacing();
            igText("Key: ");
            igSameLine(0, 5);
            
            // Show current hotkey
            char hotkey_str[64];
            preferences_hotkey_string(&prefs->launcher_hotkey, hotkey_str, sizeof(hotkey_str));
            igTextColored((ImVec4){0.2f, 0.8f, 0.4f, 1.0f}, "%s", hotkey_str);
            
            igSpacing();
            igTextDisabled("Click below and press a key to change:");
            
            static bool capturing = false;
            if (igButton(capturing ? "Press a key..." : "Change Key", (ImVec2){0, 0})) {
                capturing = true;
            }
            
            if (capturing) {
                // Capture any key press
                for (int key = ImGuiKey_Tab; key < ImGuiKey_GamepadBack; key++) {
                    if (igIsKeyPressed_Bool(key, false)) {
                        prefs->launcher_hotkey.key = key;
                        capturing = false;
                        break;
                    }
                }
            }
            
            igSpacing();
        }
        
        // Raycast Features Section
        if (igCollapsingHeader_TreeNodeFlags("Raycast-Style Features", ImGuiTreeNodeFlags_DefaultOpen)) {
            igCheckbox("Enable Calculator", &prefs->calculator_enabled);
            if (igIsItemHovered(0)) {
                igSetTooltip("Type math expressions in launcher (e.g., '2+2', '10*5')");
            }
            
            igCheckbox("Enable Clipboard History", &prefs->clipboard_history_enabled);
            if (igIsItemHovered(0)) {
                igSetTooltip("Access recent clipboard items from launcher");
            }
            
            if (prefs->clipboard_history_enabled) {
                igIndent(20.0f);
                igPushItemWidth(100);
                igSliderInt("##clipsize", &prefs->clipboard_history_size, 10, 100, "%d items", ImGuiSliderFlags_None);
                igPopItemWidth();
                igSameLine(0, 10);
                igText("History Size");
                igUnindent(20.0f);
            }
            
            igCheckbox("Enable System Commands", &prefs->system_commands_enabled);
            if (igIsItemHovered(0)) {
                igSetTooltip("Run system commands from launcher");
            }
            
            igSpacing();
        }
        
        // Save/Reset Buttons
        igSeparator();
        igSpacing();
        
        if (igButton("Save Preferences", (ImVec2){0, 0})) {
            preferences_save(prefs);
            printf("Preferences saved\n");
        }
        
        igSameLine(0, 10);
        if (igButton("Reset to Defaults", (ImVec2){0, 0})) {
            preferences_init(prefs);
            preferences_save(prefs);
            printf("Preferences reset to defaults\n");
        }
        
        igSameLine(0, 10);
        if (igButton("Close", (ImVec2){0, 0})) {
            *show_preferences = false;
        }
    }
    igEnd();
}
