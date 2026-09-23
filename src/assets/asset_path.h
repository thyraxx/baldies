#ifndef ASSET_PATH_H
#define ASSET_PATH_H

#include <stdio.h>
#include <stdbool.h>

void asset_system_init(void);
bool asset_resolve_path(const char *subpath, char *out_full_path, size_t out_size);
FILE* asset_open_file(const char *subpath, const char *mode);

#endif // ASSET_PATH_H
