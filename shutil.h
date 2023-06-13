#pragma once

#include <stddef.h>

char* sh_readFile(const char* path, size_t* out_length);
int sh_writeFile(const char* path, void* data, size_t length);
int sh_copy(const char* srcpath, const char* destpath);

char* sh_concatStrings(const char* const* arr);
char* sh_cachePath(void* data, size_t length, const char* extension);
