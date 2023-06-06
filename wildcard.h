#pragma once

#include <stdbool.h>

bool wildcard_matchPathComponent(const char* name, const char* pattern);
bool wildcard_match(const char* path, const char* pattern);

bool is_directory(const char* filename);
char** wildcard_glob(const char* path);
void wildcard_globfree(char** globs);
