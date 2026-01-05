#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

/**
 * Get the platform-specific application data directory.
 * Returns a pointer to a static buffer containing the path.
 * 
 * Linux: ~/.local/share/samfocus/
 * Windows: %APPDATA%\samfocus\
 * 
 * The directory may not exist yet - call ensure_dir_exists() first.
 */
const char* get_app_data_dir(void);

/**
 * Ensure that a directory exists, creating it if necessary.
 * Creates parent directories as needed.
 * 
 * Returns 0 on success, -1 on error.
 */
int ensure_dir_exists(const char* path);

/**
 * Join two path components with the platform-specific separator.
 * Returns a pointer to a static buffer containing the joined path.
 */
const char* path_join(const char* dir, const char* file);

#endif // PLATFORM_H
