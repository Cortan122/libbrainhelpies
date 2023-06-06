#include "wildcard.h"

#include <string.h>
#include <sys/stat.h>

#include "diagnostic.h"

#ifdef _WIN32
#include "minirent.h"
#else
#include <dirent.h>
#endif

#pragma directory("https://github.com/nothings/stb")
#include "stb_ds.h"

bool wildcard_matchPathComponent(const char* name, const char* pattern){
  // based on https://github.com/gcc-mirror/gcc/blob/master/libiberty/fnmatch.c
  // strings are terminated either by '/' or '\0'

  for(const char* p = pattern; *p != '\0' && *p != '/'; p++){
    if(*p == '*'){
      char c = *++p; // char after wildcard
      if(c == '\0' || c == '/')return true;
      for(; *name != '\0' && *name != '/'; name++){
        if(*name == c && wildcard_matchPathComponent(name, p))return true;
      }
      return false;
    }else{
      if(*name != *p)return false;
    }

    name++;
  }

  return *name == '\0' || *name == '/';
}

bool wildcard_match(const char* path, const char* pattern){
  char* next_pattern = strchr(pattern, '/');
  char* next_path = strchr(path, '/');
  bool res = wildcard_matchPathComponent(path, pattern);

  if(!res)return false;
  if(next_path == NULL && next_pattern == NULL)return true;
  if(next_path == NULL || next_pattern == NULL)return false;

  return wildcard_match(next_path+1, next_pattern+1);
}

bool is_directory(const char* filename){
  struct stat path;
  if(stat(filename, &path)){
    PERROR("failed to stat '%s'", filename);
  }
  return S_ISDIR(path.st_mode);
}

static int wildcard_globimpl(char** buffer, const char* pattern, char*** res){
  int prevlen = arrlen(*buffer);
  arrpush(*buffer, '\0');
  DIR* dir = opendir(*buffer);

  if(dir == NULL){
    PERROR("failed to open directory '%s'", *buffer);
    return -1;
  }
  errno = 0;

  struct dirent *dp = NULL;
  while((dp = readdir(dir))){

    if(!wildcard_matchPathComponent(dp->d_name, pattern))continue;

    arrsetlen(*buffer, prevlen);
    arrpush(*buffer, '/');
    memcpy(arraddnptr(*buffer, strlen(dp->d_name)), dp->d_name, strlen(dp->d_name));
    arrput(*buffer, '\0');

    char* next = strchr(pattern, '/');
    bool is_dir = is_directory(*buffer);
    if(next && is_dir){
      if(wildcard_globimpl(buffer, next+1, res))goto fail;
    }else if(!next && !is_dir){
      arrpush(*res, strdup(*buffer));
    }

    errno = 0;
  }

  if(errno){
    PERROR("failed to read directory '%s'", *buffer);
    goto fail;
  }
  if(closedir(dir))return -1;
  return 0;

  fail:;
  closedir(dir);
  return -1;
}

char** wildcard_glob(const char* path){
  char** res = NULL;
  char* buffer = NULL;
  arrpush(buffer, '.');

  if(wildcard_globimpl(&buffer, path, &res)){
    wildcard_globfree(res);
    res = NULL;
  }else{
    arrpush(res, NULL);
  }

  arrfree(buffer);
  return res;
}

void wildcard_globfree(char** globs){
  for(int i = 0; i < arrlen(globs); i++){
    free(globs[i]);
  }
  arrfree(globs);
}
