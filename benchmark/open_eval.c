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

inline void end_path(char *path, int depth) {
  path[g_offset + 2 * depth + 2] = '\0';
}

int traverse(char *path, const int fanout, const int file_cnt,
    void (*visit)(char *file)) {
  long tran_cnt = 0;
  int depth = 0;
  write_path(path, depth, 0);

  while (depth < fanout) {
    end_path(path, depth);
    // Transaction executes.
    write_path(path, depth + 1, 54); // 'f'
    write_path(path, depth + 2, 54); // 'f'
    end_path(path, depth + 2);
    int f;
    for (f = 0; f < file_cnt; ++f) {
      path[g_offset + depth * 2 + 4] = '0' + f;
      visit(path); 
      ++tran_cnt;
    }
    end_path(path, depth); // recover
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
  return tran_cnt;
} 

void print(char *path) {
  fprintf(stdout, "%s\n", path);
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("Usage: ./open_eval [path] [fanout] [file count]\n");
    return -1;
  }

  const int file_cnt = atoi(argv[3]);
  const int fanout = atoi(argv[2]);
  if (fanout > 9 || file_cnt > 9) {
    printf("Too large fanout or file count: %d or %d > 9.\n",
        fanout, file_cnt);
    return -1;
  }
  const char *prefix = argv[1];
  g_offset = strlen(prefix);

  char path[MAX_LEN];
  strcpy(path, prefix);

  long begin, end;
  begin = clock();
  long tran_cnt = traverse(path, fanout, file_cnt, print);
  end = clock();

  double sec = (double)(end - begin) / CLOCKS_PER_SEC;
  fprintf(stdout, "# Transaction Count # Time (s) # Transactions per Second\n");
  fprintf(stdout, "%ld\t%.2f\t%.2f\n", tran_cnt, sec, (double)tran_cnt / sec);
}

