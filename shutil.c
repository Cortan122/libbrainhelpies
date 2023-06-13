#include "shutil.h"

#include <stdlib.h>
#include <sys/stat.h>

#include "diagnostic.h"

char* sh_readFile(const char* path, size_t* out_length){
  struct stat stats;
  if(stat(path, &stats)){
    PERROR("stat(%s) failed", path);
    return NULL;
  }
  size_t len = stats.st_size;
  if(out_length) *out_length = len;

  FILE* file = fopen(path, "rb");
  char* data = calloc(len + 1, sizeof(char));
  if(fread(data, sizeof(char), len, file) != len){
    PERROR("fread(%s) failed", path);
    free(data);
    data = NULL;
  }
  fclose(file);

  return data;
}

int sh_writeFile(const char* path, void* data, size_t length){
  int res = 0;

  FILE* file = fopen(path, "wb");
  if(fwrite(data, sizeof(char), length, file) != length){
    PERROR("fwrite(%s) failed", path);
    res = -1;
  }
  fclose(file);

  return res;
}

int sh_copy(const char* srcpath, const char* destpath){
  size_t len;
  char* data = sh_readFile(srcpath, &len);
  if(data == NULL)return -1;

  int res = sh_writeFile(destpath, data, len);
  free(data);
  return res;
}

// todo: add a preprocessor wrapper
char* sh_concatStrings(const char* const* arr){
  size_t total_len = 0;
  for(int i = 0; arr[i]; i++){
    total_len += strlen(arr[i]);
  }

  char* res = malloc(total_len+1);
  res[0] = '\0';
  for(int i = 0; arr[i]; i++){
    strcat(res, arr[i]);
  }
  return res;
}

char* sh_cachePath(void* data, size_t length, const char* extension){
  // todo: arena allocator
  // todo: sha
  // THIS IS A PLACEHOLDER
  char hash[32+1] = {0};
  for(size_t i = 0; i < length; i++){
    char byte = ((char*)data)[i];
    hash[(i&0xf)*2+0] ^= (byte&0x0f)>>0;
    hash[(i&0xf)*2+1] ^= (byte&0xf0)>>4;
  }
  for(int i = 0; i < 32; i++){
    hash[i] = "0123456789abcdef"[(int)hash[i]];
  }

  // todo: mkdir
  return sh_concatStrings((const char*[]){
    #ifndef _WIN32
    getenv("HOME"),
    "/.cache/",
    #else
    getenv("LOCALAPPDATA"),
    "/"
    #endif
    "libbrainhelpies/",
    hash,
    extension,
    NULL
  });
}
