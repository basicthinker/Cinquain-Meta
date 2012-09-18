#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

// depth is the current valid depth
inline void end_path(char *path, int depth) {
  path[g_offset + 2 * depth + 2] = '\0';
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stdout, "Usage: ./rmdir_eval [path] [fanout]\n");
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

  int depth;
  for (depth = 0; depth < fanout; ++depth) {  
    write_path(path, depth, fanout - 1);
  }
  --depth;
  
  long tran_cnt = 0;
  long begin, end;
  begin = clock();
  while (depth >= 0) {
    end_path(path, depth);
    // Transaction executes.
    fprintf(stdout, "%s\n", path);
//    if (mkdir(path, 0755) != 0) {
//      fprintf(stderr, "Error: creating dir %s.\n", path);
//      return -1;
//    }
    ++tran_cnt;
    int cur, i;
    for (i = 0; i <= depth; ++i) {
      cur = read_path(path, i);
      if (cur > 0) {
        dec_path(path, i);
        break;
      } else {
        write_path(path, i, fanout - 1);        
      }
    }
    if (i == depth + 1) {
      --depth;
    }
  }
  end = clock();

  double sec = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf(stdout, "# Evaluation of rmdir()\n");
  fprintf(stdout, "# Transaction Count # Time (s) # Transactions per Second\n");
  fprintf(stdout, "%ld\t%.2f\t%.2f\n", tran_cnt, sec, (double)tran_cnt / sec);
  return 0;
}

