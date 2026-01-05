#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <shlobj.h>
    #include <direct.h>
    #define PATH_SEP '\\'
    #define mkdir_portable(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <errno.h>
    #define PATH_SEP '/'
    #define mkdir_portable(path) mkdir(path, 0755)
#endif

static char app_data_dir_buf[512];
static char path_join_buf[1024];

const char* get_app_data_dir(void) {
    static int initialized = 0;
    
    if (initialized) {
        return app_data_dir_buf;
    }
    
#ifdef PLATFORM_WINDOWS
    // Use SHGetFolderPath to get %APPDATA%
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, app_data_dir_buf) != S_OK) {
        // Fallback to environment variable
        const char* appdata = getenv("APPDATA");
        if (appdata) {
            snprintf(app_data_dir_buf, sizeof(app_data_dir_buf), "%s\\samfocus", appdata);
        } else {
            // Last resort: use current directory
            strcpy(app_data_dir_buf, ".\\samfocus");
        }
    } else {
        strcat(app_data_dir_buf, "\\samfocus");
    }
#else
    // Linux: Use XDG_DATA_HOME or fallback to ~/.local/share
    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && xdg_data_home[0] == '/') {
        snprintf(app_data_dir_buf, sizeof(app_data_dir_buf), 
                 "%s/samfocus", xdg_data_home);
    } else {
        const char* home = getenv("HOME");
        if (home) {
            snprintf(app_data_dir_buf, sizeof(app_data_dir_buf), 
                     "%s/.local/share/samfocus", home);
        } else {
            // Fallback to current directory
            strcpy(app_data_dir_buf, "./samfocus");
        }
    }
#endif
    
    initialized = 1;
    return app_data_dir_buf;
}

int ensure_dir_exists(const char* path) {
    if (!path || path[0] == '\0') {
        return -1;
    }
    
    // Try to create the directory
    if (mkdir_portable(path) == 0) {
        return 0;  // Successfully created
    }
    
#ifdef PLATFORM_WINDOWS
    // On Windows, check if directory already exists
    DWORD attrs = GetFileAttributesA(path);
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return 0;  // Already exists
    }
#else
    // On Linux, check errno
    if (errno == EEXIST) {
        return 0;  // Already exists
    }
    
    // If parent doesn't exist, create parent first
    if (errno == ENOENT) {
        char parent[512];
        strncpy(parent, path, sizeof(parent) - 1);
        parent[sizeof(parent) - 1] = '\0';
        
        // Find last path separator
        char* last_sep = strrchr(parent, PATH_SEP);
        if (last_sep && last_sep != parent) {
            *last_sep = '\0';
            
            // Recursively create parent
            if (ensure_dir_exists(parent) != 0) {
                return -1;
            }
            
            // Try again
            if (mkdir_portable(path) == 0) {
                return 0;
            }
            
            if (errno == EEXIST) {
                return 0;
            }
        }
    }
#endif
    
    return -1;
}

const char* path_join(const char* dir, const char* file) {
    if (!dir || !file) {
        return NULL;
    }
    
    size_t dir_len = strlen(dir);
    int needs_sep = (dir_len > 0 && dir[dir_len - 1] != PATH_SEP);
    
    snprintf(path_join_buf, sizeof(path_join_buf), 
             "%s%c%s", dir, needs_sep ? PATH_SEP : '\0', needs_sep ? file : file);
    
    return path_join_buf;
}
