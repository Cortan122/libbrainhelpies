#include <stdio.h>

#pragma directory("../")
#include "wildcard.h"
#include "VFS.h"

#pragma directory("https://github.com/nothings/stb")
#include "stb_ds.h"

#define DEBUG(f, a1, a2) printf(#f "(" #a1 ", " #a2 ") = %d\n", f(a1, a2));

int main(){
  DEBUG(wildcard_match, "hello/hi", "*");
  DEBUG(wildcard_match, "hello/hi", "*/*");
  DEBUG(wildcard_match, "hello.txt", "*/*");
  DEBUG(wildcard_match, "demo.zip", "*.zip");
  DEBUG(wildcard_match, "demo.txt", "*.zip");
  DEBUG(wildcard_match, "demo.zip", "dem*.zip");

  char** paths = wildcard_glob("*.c");
  wildcard_globfree(paths);

  VFS* zip = VFS_create("assets/demo.zip", VFS_ZIP);

  VFS_findFiles(zip, "*.c");
  for(int i = 0; i < arrlen(zip->files); i++){
    printf("found path %s\n", zip->files[i].path);
  }
  VFS_commitRead(zip);

  VFS_writeMemory(zip, "hello.txt", "hello world!\n(i am a file, you know)\n", 37);

  VFS_delete(zip);
  return 0;
}
