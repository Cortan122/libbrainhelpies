#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#else
#include <unistd.h>
#endif

#pragma directory("https://github.com/nemequ/munit")
#define MUNIT_ENABLE_ASSERT_ALIASES
#include "munit.h"

#pragma region wildcard
#pragma directory("../")
#include "wildcard.h"

MunitResult test_wildcard_match(const MunitParameter* params, void* user_data){
  (void)params;
  (void)user_data;

  assert_int(wildcard_match("hello/hi", "*"),        ==, false);
  assert_int(wildcard_match("hello/hi", "*/*"),      ==, true);
  assert_int(wildcard_match("hell.txt", "*/*"),      ==, false);
  assert_int(wildcard_match("demo.zip", "*.zip"),    ==, true);
  assert_int(wildcard_match("demo.txt", "*.zip"),    ==, false);
  assert_int(wildcard_match("demo.zip", "dem*.zip"), ==, true);
  assert_int(wildcard_match("demo.ZIP", "dem*.zip"), ==, false);

  return MUNIT_OK;
}

MunitResult test_wildcard_component(const MunitParameter* params, void* user_data){
  (void)params;
  (void)user_data;

  assert_int(wildcard_matchPathComponent("hi", "hi"),             ==, true);
  assert_int(wildcard_matchPathComponent("hi", "lol"),            ==, false);

  assert_int(wildcard_matchPathComponent("hello/hi", "*/*"),      ==, true);
  assert_int(wildcard_matchPathComponent("hello/hi", "*/nAJSAH"), ==, true);
  assert_int(wildcard_matchPathComponent("hell.txt", "*/*"),      ==, true);
  assert_int(wildcard_matchPathComponent("hi/lower", "hi"),       ==, true);

  assert_int(wildcard_matchPathComponent("hi_lower", "*_*"),      ==, true);
  assert_int(wildcard_matchPathComponent("hi-lower", "*_*"),      ==, false);

  assert_int(wildcard_matchPathComponent("hi_lower", "hi*"),      ==, true);
  assert_int(wildcard_matchPathComponent("fhilower", "hi*"),      ==, false);

  assert_int(wildcard_matchPathComponent("hi_lower", "*er"),      ==, true);
  assert_int(wildcard_matchPathComponent("hilowerf", "*er"),      ==, false);

  assert_int(wildcard_matchPathComponent("hi_lower", "hi*er"),    ==, true);
  assert_int(wildcard_matchPathComponent("fhilower", "hi*er"),    ==, false);
  assert_int(wildcard_matchPathComponent("hilowerf", "hi*er"),    ==, false);

  assert_int(wildcard_matchPathComponent("fhilower", "f*i*o*e*"), ==, true);
  assert_int(wildcard_matchPathComponent("hilowerf", "f*i*o*e*"), ==, false);
  assert_int(wildcard_matchPathComponent("ohifrlwe", "f*i*o*e*"), ==, false);
  assert_int(wildcard_matchPathComponent("loifwrhe", "f*i*o*e*"), ==, false);
  assert_int(wildcard_matchPathComponent("fierlhwo", "f*i*o*e*"), ==, false);

  assert_int(wildcard_matchPathComponent("demo.zip", "*.zip"),    ==, true);
  assert_int(wildcard_matchPathComponent("demo.txt", "*.zip"),    ==, false);
  assert_int(wildcard_matchPathComponent("demo.zip", "dem*.zip"), ==, true);
  assert_int(wildcard_matchPathComponent("demo.ZIP", "dem*.zip"), ==, false);

  return MUNIT_OK;
}

MunitResult test_wildcard_glob(const MunitParameter* params, void* user_data){
  (void)params;
  (void)user_data;

  int length = 0;
  char** paths = wildcard_glob("*");
  for(char** pathptr = paths; *pathptr; pathptr++){
    length++;
    char* path = *pathptr;
    assert_true(access(path, R_OK) == 0);
  }

  int other_len = 0;
  char** other_paths = calloc(length+1, sizeof(char*));
  for(char** pathptr = paths; *pathptr; pathptr++){
    char* path = *pathptr;
    if(wildcard_match(path, "./*.c")){
      other_paths[other_len++] = path;
    }
  }

  int length2 = 0;
  char** paths2 = wildcard_glob("*.c");
  for(char** pathptr = paths2; *pathptr; pathptr++){
    length2++;
    char* path = *pathptr;
    assert_true(wildcard_match(path, "./*.c"));
    assert_string_equal(path, other_paths[length2-1]);
  }
  assert_int(length2, <=, length);
  assert_int(length2, ==, other_len);

  free(other_paths);
  wildcard_globfree(paths);
  wildcard_globfree(paths2);
  return MUNIT_OK;
}

#pragma endregion

#pragma region vfs
#pragma directory("../")
#include "VFS.h"

MunitResult test_vfs_zipread(const MunitParameter* params, void* user_data){
  (void)params;
  (void)user_data;

  VFS* zip = VFS_create("../assets/demo.zip", VFS_ZIP);
  VFS_findFiles(zip, "*.c");
  VFS_commitRead(zip);

  static const char* rom[] = {
    "diagnostic.c",
    "main.c",
    "VFS.c",
  };

  assert_size(zip->files_count, ==, 3);
  for(size_t i = 0; i < zip->files_count; i++){
    assert_string_equal(zip->files[i].path, rom[i]);
  }

  VFS_delete(zip);
  return MUNIT_OK;
}

MunitResult test_vfs_zipwrite(const MunitParameter* params, void* user_data){
  (void)params;
  (void)user_data;

  int number = munit_rand_int_range(0, 10000);
  char* rom = "hello world!\n(i am a file, you know)\n%d\n";
  char* filename = "hello.txt";
  char* demo_archive = "../assets/demo.zip";

  char buf[256] = {0};
  snprintf(buf, sizeof(buf)-1, rom, number);

  {
    VFS* zip = VFS_create(demo_archive, VFS_ZIP);
    VFS_writeMemory(zip, filename, buf, strlen(buf));
    VFS_delete(zip);
  }

  {
    VFS* zip = VFS_create(demo_archive, VFS_ZIP);

    // todo: maybe make a direct VFS_readFile() function?
    VFS_findFiles(zip, filename);
    VFS_commitRead(zip);
    assert_int(zip->files_count, ==, 1);
    assert_string_equal(zip->files[0].path, filename);
    assert_true(zip->files[0].mem_valid);

    assert_size(zip->files[0].mem_length, ==, strlen(buf));
    assert_memory_equal(strlen(buf), zip->files[0].mem_buffer, buf);

    VFS_delete(zip);
  }

  return MUNIT_OK;
}

#pragma endregion

MunitTest tests[] = {
  {.name = "/wildcard/match", .test = test_wildcard_match},
  {.name = "/wildcard/component", .test = test_wildcard_component},
  {.name = "/wildcard/glob", .test = test_wildcard_glob},

  {.name = "/vfs/zip_read", .test = test_vfs_zipread},
  {.name = "/vfs/zip_write", .test = test_vfs_zipwrite},
  {0}
};

MunitSuite suite = {
  .prefix = "/brain-helpies",
  .tests = tests,
  .iterations = 1,
};

int main(int argc, char** argv){
  return munit_suite_main(&suite, NULL, argc, argv);
}
