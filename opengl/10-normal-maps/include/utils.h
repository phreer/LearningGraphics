#ifndef __UTILS_H__
#define __UTILS_H__
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <vector>

struct FileContent {
  char *content;
  size_t capability;
  size_t length; // length of the file
};

static inline int LoadFile(const char *filename, struct FileContent *result) {
  size_t file_size = 0;
  size_t read_count = 0;
  char *content = NULL;

  FILE *f = fopen(filename, "r");
  if (!f) {
    perror("failed to open file");
    return -1;
  }
  
  fseek(f, 0L, SEEK_END);
  file_size = ftell(f);
  fseek(f, 0L, SEEK_SET);

  content = (char *) malloc(file_size + 1);
  if (!content) {
    perror("failed to allocate memory to load file content");
    fclose(f);
    return -1;
  }

  read_count = fread(content, sizeof(char), file_size, f);
  if (read_count != file_size) {
    if (feof(f)) {
      fprintf(stderr, "unexpected EOF encountered\n");
    } else {
      perror("failed to load file content");
    }
    fclose(f);
    free(content);
    return -1;
  }

  fclose(f);
  content[file_size] = '\0';
  *result = {content, file_size, file_size + 1};
  return 0;
}


static inline int StoreFile(const char *filename, const char *data, size_t size) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    perror("failed to open file");
    return -1;
  }
  
  fwrite(data, 1, size, f);
  fclose(f);
  return 0;
}
#endif // __SHADER_H__
