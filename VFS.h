#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef enum VFSType {
  VFS_NONE,
  // VFS_FILE,
  VFS_ZIP,
  // VFS_GIT,
  // VFS_NEOCITIES,
} VFSType;

typedef struct VFSFile {
  struct VFS* system;
  bool is_valid;
  bool path_owned;
  char* path;
  time_t date;

  bool mem_valid;
  bool mem_owned;
  void* mem_buffer;
  size_t mem_length;

  bool disk_valid;
  char* disk_path;

  bool destpath_owned;
  char* destpath;
} VFSFile;

typedef struct VFS {
  VFSType type;
  bool can_read;
  bool can_write;
  char* uri;
  VFSFile* files;     // stbarr
  size_t files_count; // redundant output interface

  void* opaque;
} VFS;

VFS* VFS_create(char* uri, VFSType type);
void VFS_delete(VFS* vfs);

int VFS_findFiles(VFS* vfs, const char* path);
int VFS_findFilesWithPattren(VFS* vfs, const char* path, const char* pattern);
int VFS_commitRead(VFS* vfs);

int VFS_writeMemory(VFS* vfs, char* file, void* buffer, size_t length);
int VFS_commitWrite(VFS* vfs);

VFSFile* VFSFile_makeMemory(VFS* vfs, const char* name, size_t length);
int VFSFile_ensureMemory(VFSFile* file);
void VFSFile_computeDestination(VFSFile* file, const char* pattern);
void VFSFile_delete(VFSFile* file);
