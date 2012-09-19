#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
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

inline void dec_path(char *path, int depth) {
  --path[g_offset + 2 * depth + 1];
}

inline void end_path(char *path, int depth) {
  path[g_offset + 2 * depth + 2] = '\0';
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stdout, "Usage: ./mkdir_eval [path] [fanout]\n");
    return -1;
  }

  int fanout = atoi(argv[2]);
  if (fanout > 9) {
    fprintf(stderr, "Too large fanout: %d > 9.\n", fanout);
    return -1;
  }
  char *prefix = argv[1];
  g_offset = strlen(prefix);

  char path[MAX_LEN];
  strcpy(path, prefix);

  int depth = 0;
  write_path(path, depth, 0);

  long tran_cnt = 0;
  struct timeval begin, end;
  gettimeofday(&begin, NULL);
  while (depth < fanout) {
    end_path(path, depth);
    // Transaction executes.
    if (mkdir(path, 0755) != 0) {
      fprintf(stderr, "Error: creating dir %s.\n", path);
      return -1;
    }
    ++tran_cnt;
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
  gettimeofday(&end, NULL);

  double sec = end.tv_sec - begin.tv_sec + (double)(end.tv_usec - begin.tv_usec) / 1000000;
  fprintf(stdout, "# Evaluation of mkdir()\n"
         "# Transaction Count # Time (s) # Transactions per Second\n"
         "%ld\t%.2f\t%.2f\n", tran_cnt, sec, tran_cnt / sec);
  return 0;
}


