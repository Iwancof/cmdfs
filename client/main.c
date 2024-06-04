#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 31

#define _GNU_SOURCE // for pipes

#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <fuse/fuse.h>
#include <fuse/fuse_common.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FILE *debug;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

__attribute__((format(printf, 1, 2))) void dbg(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  vfprintf(debug, fmt, args);
  fflush(debug);

  va_end(args);

}

char *(*exec_cmd)(char *sub_cmd, int num_args, ...);

char *bin_path = "/home/iwancof/WorkSpace/cmdfs/target/main";
char *simple_exec(char *sub_cmd, int num_args, ...) {
  va_list args;

  size_t cmd_size = 4096, cmd_len = 0;
  char *cmd = (char *)malloc(cmd_size);

  va_start(args, num_args);

  cmd_len +=
      snprintf(&cmd[cmd_len], cmd_size - cmd_len, "%s %s", bin_path, sub_cmd);

  for (int i = 0; i < num_args; i++) {
    char *arg = va_arg(args, char *);
    cmd_len += snprintf(&cmd[cmd_len], cmd_size - cmd_len, " %s", arg);

    if (cmd_len >= cmd_size) {
      printf("cmd is too long\n");
      exit(1);
    }
  }

  va_end(args);

  dbg("cmd: %s\n", cmd);

  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    return NULL;
  }

  size_t size = 4096, num_remain = size, num_read = 0;
  char *buf = (char *)malloc(size);

  while (1) {
    if (num_remain == 0) {
      num_remain = size;
      size *= 2;
      buf = (char *)realloc(buf, size);
    }

    char *ret = fgets(buf + num_read, num_remain, fp);
    if (ret == NULL) {
      break;
    }

    size_t len = strlen(ret);
    num_read += len;
    num_remain -= len;
  }

  pclose(fp);

  return buf;
}

int cmdfs_getattr(const char *path, struct stat *stbuf) {
  char *result = exec_cmd("getattr", 1, path);
  char *token, *saveptr;

  token = strtok_r(result, ",", &saveptr);
  if (token == NULL) {
    return -EIO;
  }

  int ret;
  sscanf(token, "ret=%d", &ret);

  char buf[sizeof(struct stat)];
  token = strtok_r(NULL, ",", &saveptr);
  if (token == NULL) {
    return -EIO;
  }

  for (size_t i = 0; i < sizeof(struct stat); i++) {
    sscanf(&token[i * 2], "%02hhx", &buf[i]);
  }

  memcpy(stbuf, buf, sizeof(struct stat));

  return ret;
}

int cmdfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi) {
  char *result = exec_cmd("readdir", 1, path);
  char *token, *saveptr;

  token = strtok_r(result, ",", &saveptr);
  if (token == NULL) {
    return -EIO;
  }

  int ret;
  sscanf(token, "ret=%d", &ret);

  token = strtok_r(NULL, ",", &saveptr);
  if (token == NULL) {
    return -EIO;
  }

  int len;
  sscanf(token, "len=%d", &len);

  for (int i = 0; i < len; i++) {
    token = strtok_r(NULL, ",", &saveptr);
    if (token == NULL) {
      return -EIO;
    }

    filler(buf, token, NULL, offset);
  }

  return ret;
}

int main(int argc, char *argv[]) {
  struct fuse_operations op = {
      .getattr = cmdfs_getattr,
      .readdir = cmdfs_readdir,
  };

  // exec test
  // char *b = simple_exec("getattr", 1, "/aa");
  // printf("%s\n", b);

  exec_cmd = simple_exec;

  debug = fopen("/home/iwancof/debuglog", "w");

  // struct stat s;
  // cmdfs_getattr("/", &s);

  dbg("start\n");

  return fuse_main(argc, argv, &op, NULL);
}
