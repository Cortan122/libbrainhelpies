#include "VFS.h"

#include <assert.h>
#include <stdio.h>

#include "diagnostic.h"
#include "wildcard.h"

#pragma directory("https://github.com/nothings/stb")
#include "stb_ds.h"

#pragma comment(lib, "zip")
#include <zip.h>

#define VFS_NOTSUPPORTED(vfs) fprintf(stderr, INFO"VFS of type %d does not support operation %s\n", vfs->type, __func__)
#define VFS_READABLE(vfs) \
  if(!vfs->can_read){     \
    fprintf(stderr, ERROR"a VFS must be readable to support operation %s\n", __func__); \
    return -1;            \
  }
#define VFS_WRITABLE(vfs) \
  if(!vfs->can_write){    \
    fprintf(stderr, ERROR"a VFS must be writable to support operation %s\n", __func__); \
    return -1;            \
  }

static void* VFSzip_makeOpaque(char* uri){
  int err;
  zip_t* zip = zip_open(uri, ZIP_CREATE, &err);
  if(zip == NULL){
    zip_error_t error;
    zip_error_init_with_code(&error, err);
    fprintf(stderr, ERROR"cannot open zip archive '%s': %s\n", uri, zip_error_strerror(&error));
    zip_error_fini(&error);
    return NULL;
  }

  return zip;
}

static int VFSzip_findFiles(VFS* vfs, const char* pattern){
  assert(vfs->type == VFS_ZIP);
  zip_t* zip = vfs->opaque;
  int64_t numfiles = zip_get_num_entries(zip, 0);

  for(int64_t i = 0; i < numfiles; i++){
    zip_stat_t stats;
    zip_stat_index(zip, i, ZIP_FL_ENC_RAW, &stats);
    assert((stats.valid & ZIP_STAT_NAME) && (stats.valid & ZIP_STAT_SIZE) && (stats.valid & ZIP_STAT_MTIME));

    if(wildcard_match(stats.name, pattern)){
      VFSFile* vfsfile = VFSFile_makeMemory(vfs, stats.name, stats.size);
      vfsfile->date = stats.mtime;

      zip_file_t* file = zip_fopen_index(zip, i, 0);
      zip_fread(file, vfsfile->mem_buffer, stats.size);
      zip_fclose(file);
    }
  }

  return 0;
}

static int VFSzip_writeMemory(VFS* vfs, char* filename, void* buffer, size_t length){
  assert(vfs->type == VFS_ZIP);
  zip_t* zip = vfs->opaque;
  zip_source_t* s = zip_source_buffer(zip, buffer, length, 0);

  if(s == NULL || zip_file_add(zip, filename, s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0){
    zip_source_free(s);
    fprintf(stderr, ERROR"failed to add file '%s' to zip: %s\n", filename, zip_strerror(zip));
    return -1;
  }
  return 0;
}

VFS* VFS_create(char* uri, VFSType type){
  VFS* res = calloc(1, sizeof(VFS));
  res->type = type;
  res->uri = uri;

  switch(type){
    case VFS_ZIP: {
      res->can_read = true;
      res->can_write = true;

      res->opaque = VFSzip_makeOpaque(uri);
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(res);
  }

  if(res->opaque == NULL){
    free(res);
    return NULL;
  }else{
    return res;
  }
}

void VFS_delete(VFS* vfs){
  switch(vfs->type){
    case VFS_ZIP: {
      if(zip_close(vfs->opaque)){
        fprintf(stderr, ERROR"failed to write zip: %s\n", zip_strerror(vfs->opaque));
        zip_discard(vfs->opaque);
      }
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  for(int i = 0; i < arrlen(vfs->files); i++){
    VFSFile_delete(&vfs->files[i]);
  }
  arrfree(vfs->files);

  free(vfs);
}

int VFS_findFiles(VFS* vfs, char* path){
  VFS_READABLE(vfs);

  switch(vfs->type){
    case VFS_ZIP: {
      return VFSzip_findFiles(vfs, path);
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  vfs->files_count = arrlen(vfs->files);
  return 0;
}

int VFS_commitRead(VFS* vfs){
  VFS_READABLE(vfs);

  switch(vfs->type){
    case VFS_ZIP: {
      // do nothing!
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  vfs->files_count = arrlen(vfs->files);
  return 0;
}

int VFS_writeMemory(VFS* vfs, char* file, void* buffer, size_t length){
  VFS_WRITABLE(vfs);

  switch(vfs->type){
    case VFS_ZIP: {
      return VFSzip_writeMemory(vfs, file, buffer, length);
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  return 0;
}

int VFS_commitWrite(VFS* vfs){
  VFS_WRITABLE(vfs);

  switch(vfs->type){
    case VFS_ZIP: {
      // do nothing!
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  return 0;
}

VFSFile* VFSFile_makeMemory(VFS* vfs, const char* name, size_t length){
  VFSFile file = {0};
  file.system = vfs;
  file.is_valid = true;
  file.mem_valid = true;
  file.mem_owned = true;
  file.mem_length = length;
  file.mem_buffer = calloc(length+1, 1);
  file.path_owned = true;
  file.path = strdup(name);
  arrpush(vfs->files, file);

  // note: this pointer is valid only until the next call
  // the actual file is valid much longer than that!!
  return &vfs->files[arrlen(vfs->files)-1];
}

void VFSFile_delete(VFSFile* file){
  if(file->path_owned){
    free(file->path);
  }
  if(file->mem_owned){
    free(file->mem_buffer);
  }
}
