#define _FILE_OFFSET_BITS 64

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

__attribute__((noreturn)) void die(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
  exit(1);
}

void target_getattr(int argc, char *argv[]) {
  struct stat st;

  if (argc < 1) {
    die("not enough arguments(getattr)");
  }

  char *path = argv[0];

  int ret = stat(path, &st);
  char buf[sizeof(struct stat)];
  memcpy(buf, &st, sizeof(struct stat));

  printf("ret=%d,", ret);
  for (size_t i = 0; i < sizeof(struct stat); i++) {
    printf("%02hhx", buf[i]);
  }

  fflush(stdout);

  exit(0);
}

void target_readdir(int argc, char *argv[]) {
  // get directory entries

  if (argc < 1) {
    die("not enough arguments(readdir)");
  }

  char *path = argv[0];
  int ret, num_of_files = 0;
  struct dirent *dp;
  char *files[1024]; // max 1024 files

  DIR *dir = opendir(path);
  if (dir == NULL) {
    ret = -errno;
    goto RESULT;
  }

  while ((dp = readdir(dir)) != NULL) {
    files[num_of_files] = dp->d_name;
    num_of_files++;
  }

  closedir(dir);

RESULT:
  printf("ret=%d,", ret);
  printf("len=%d,", num_of_files);
  for (int i = 0; i < num_of_files; i++) {
    printf("%s,", files[i]);
  }

  fflush(stdout);
  exit(0);
}

void target_open(int argc, char *argv[]) {
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    die("Usage: ./main cmd");
  }

  char *cmd = argv[1];

  if (strcmp(cmd, "getattr") == 0) {
    target_getattr(argc - 2, argv + 2);
  } else if (strcmp(cmd, "readdir") == 0) {
    target_readdir(argc - 2, argv + 2);
  } else {
    die("unknown command");
  }
}
