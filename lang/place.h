#ifndef LANG_PLACE_H
#define LANG_PLACE_H

#include <stddef.h>

/* override is optional. family_dir is the tool directory. cwd is the launch directory. */
int place_dir(char *out, size_t cap, const char *family_dir, const char *name, const char *override, const char *cwd);
int place_tool(char *out, size_t cap, const char *name, const char *override);
/* Relative paths become absolute from the current directory. Absolute paths stay absolute. */
int place_absolute(char *out, size_t cap, const char *path);

#endif
