#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LEN 128

int g_offset = 0;

inline void write_path(char *path, int depth, int value) {
  path[g_offset + 2 * depth] = '/';
  path[g_offset + 2 * depth + 1] = '0' + value; 
}

inline int read_path(char *path, int depth) {
  return path[g_offset + 2 * depth + 1] - '0';
}

inline void inc_path(char *path, int depth) {
  ++path[g_offset + 2 * depth + 1];
}

inline void end_path(char *path, int depth) {
  path[g_offset + 2 * depth + 2] = '\0';
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: ./meta_eval [path] [fanout] [num_loops]\n");
    return -1;
  }

  int fanout = atoi(argv[2]);
  if (fanout > 9) {
    printf("Too large fanout: %d > 9.\n", fanout);
    return -1;
  }
  int n_loops = atoi(argv[3]);
  char *prefix = argv[1];
  g_offset = strlen(prefix);

  char path[MAX_LEN];
  strcpy(path, prefix);

  int depth = 0;
  write_path(path, depth, 0);

  while (depth < fanout) {
    end_path(path, depth);
    if (mkdir(path, 0755) != 0) {
      fprintf(stderr, "Error: creating dir %s.\n", path);
      return -1;
    }
    inc_path(path, 0);
    int cur, i;
    for (i = 0; i < depth; ++i) {
      cur = read_path(path, i);
      if (cur < fanout) {
        break;
      } else {
        write_path(path, i, 0);
        inc_path(path, i + 1);
      }
    }
    if (i == depth && read_path(path, i) == fanout) {
      write_path(path, i, 0);
      ++depth;
      write_path(path, depth, 0);
    }
  }
}

