#include "VFS.h"

#include <assert.h>
#include <stdio.h>

#include "diagnostic.h"
#include "wildcard.h"
#include "shutil.h"

#pragma directory("https://github.com/nothings/stb")
#include "stb_ds.h"

#pragma comment(lib, "zip")
#include <zip.h>

#define VFS_NOTSUPPORTED(vfs) fprintf(stderr, INFO"VFS of type %d does not support operation %s\n", vfs->type, __func__)
#define VFS_READABLE(vfs) \
  if(!(vfs)->can_read){   \
    fprintf(stderr, ERROR"a VFS must be readable to support operation %s\n", __func__); \
    return -1;            \
  }
#define VFS_WRITABLE(vfs) \
  if(!(vfs)->can_write){  \
    fprintf(stderr, ERROR"a VFS must be writable to support operation %s\n", __func__); \
    return -1;            \
  }
#define VFSFILE_VALID(file) \
  if(!(file)->is_valid){    \
    fprintf(stderr, ERROR"trying operation %s on an invalid file '%s'\n", __func__, (file)->path); \
    return -1;              \
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

int VFS_findFiles(VFS* vfs, const char* path){
  VFS_READABLE(vfs);
  int res = 0;

  switch(vfs->type){
    case VFS_ZIP: {
      res = VFSzip_findFiles(vfs, path);
    } break;

    case VFS_NONE:
    VFS_NOTSUPPORTED(vfs);
  }

  vfs->files_count = arrlen(vfs->files);
  return res;
}

int VFS_findFilesWithPattren(VFS* vfs, const char* path, const char* pattern){
  VFS_READABLE(vfs);

  int old_count = arrlen(vfs->files);
  int res = VFS_findFiles(vfs, path);
  int new_count = arrlen(vfs->files);

  for(int i = old_count; i < new_count; i++){
    VFSFile_computeDestination(&vfs->files[i], pattern);
  }

  return res;
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
  // note: the lifetime of [buffer] must be longer then the lifelime of [vfs], if using VFS_ZIP

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

void VFSFile_computeDestination(VFSFile* file, const char* pattern){
  if(strchr(pattern, '*') == NULL){
    file->destpath_owned = false;
    file->destpath = (char*)pattern;
    return;
  }

  const char* name_start = strrchr(file->path, '/');
  if(name_start == NULL)name_start = file->path;
  else name_start += 1;
  const char* name_end = strrchr(file->path, '.');
  const char* output_star = strrchr(pattern, '*');

  int prefix_len = output_star - pattern;
  int name_len = name_end - name_start;
  size_t length = strlen(pattern) + prefix_len + name_len + 1;
  file->destpath_owned = true;
  file->destpath = calloc(length, sizeof(char));

  // [pattern..output_star][name_start..name_end][output_star+1..]
  snprintf(file->destpath, length, "%.*s%.*s%s", prefix_len, pattern, name_len, name_start, output_star+1);
}

int VFSFile_ensureMemory(VFSFile* file){
  VFSFILE_VALID(file);

  if(file->mem_valid)return 0;

  if(!file->disk_valid){
    fprintf(stderr, ERROR"we have a file with no valid source '%s'\n", file->path);
    return -1;
  }

  file->mem_buffer = sh_readFile(file->disk_path, &file->mem_length);
  if(file->mem_buffer == NULL)return -1;
  file->mem_valid = true;

  return 0;
}

int VFSFile_ensureDisk(VFSFile* file){
  VFSFILE_VALID(file);

  if(file->disk_valid)return 0;

  if(!file->mem_valid){
    fprintf(stderr, ERROR"we have a file with no valid source '%s'\n", file->path);
    return -1;
  }

  // todo: file->system->uri + file->path?
  file->disk_path = sh_cachePath(file->mem_buffer, file->mem_length, "");
  sh_writeFile(file->disk_path, file->mem_buffer, file->mem_length);

  return 0;
}

void VFSFile_delete(VFSFile* file){
  if(file->path_owned){
    free(file->path);
  }
  if(file->mem_owned){
    free(file->mem_buffer);
  }
  if(file->destpath_owned){
    free(file->destpath);
  }
  if(file->disk_valid){
    free(file->disk_path);
  }
}
