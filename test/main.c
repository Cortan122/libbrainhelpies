#include <stdio.h>

#pragma directory("../")
#include "VFS.h"
#include "shutil.h"

int main(){
  printf("path %s\n", sh_cachePath("some thing", 10, ""));

  VFS* zip = VFS_create("../assets/demo.zip", VFS_ZIP);
  VFS* dest = VFS_create("out.zip", VFS_ZIP);

  VFS_findFilesWithPattren(zip, "*.c", "c_files/*.c");
  VFS_findFilesWithPattren(zip, "*.h", "headers/*.h");
  VFS_findFilesWithPattren(zip, "hello.txt", "world.txt");
  VFS_commitRead(zip);
  for(size_t i = 0; i < zip->files_count; i++){
    printf("found path %s -> %s\n", zip->files[i].path, zip->files[i].destpath);

    VFSFile_ensureMemory(&zip->files[i]);
    VFS_writeMemory(dest, zip->files[i].destpath, zip->files[i].mem_buffer, zip->files[i].mem_length);
  }
  VFS_commitWrite(dest);

  VFS_delete(dest);
  VFS_delete(zip);
  return 0;
}
