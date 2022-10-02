
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctime>

#include "union_find.h"

double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

// returns time taken to execute test
double test_alg_1(uint32_t w, uint32_t h) {
  srand(0);

  char* board = (char*) malloc(w * h * sizeof(char));

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      switch (rand() % 4) {
        case 0:
          board[y * w + x] = ' ';
          break;
        case 1:
          board[y * w + x] = '/';
          break;
        case 2:
          board[y * w + x] = '\\';
          break;
        case 3:
          board[y * w + x] = 'X';
          break;
        default:
          // rand range can't be outside the range
          assert(0);
      }
    }
  }

  // time union find algorithm parts
  struct timespec start, end;

  clock_gettime(CLOCK_MONOTONIC, &start);

  /*
   * each tile of borad is partitioned into four segments, which may or may
   * not be connected to each other, depending on the type of tile
   *
   *  partition of tile:
   *   \  0  /
   *    \   /
   *     \ /
   *   3  X   1
   *     / \
   *    /   \
   *   /  2  \
   *
   */
  UnionFind<uint32_t> uf(4 * w * h);

  for (uint32_t y = 0; y < h; y++) {
    for (uint32_t x = 0; x < w; x++) {
      uint32_t idx = y * w + x;
      char tile = board[idx];
      // 4 segments per tile
      idx *= 4;

      if (tile == ' ' || tile == '\\') {
        // join 0 to 1 and 2 to 3
        uf.Union(idx + 0, idx + 1);
        uf.Union(idx + 2, idx + 3);
      }
      if (tile == ' ' || tile == '/') {
        // join 0 to 3 and 1 to 2
        uf.Union(idx + 0, idx + 3);
        uf.Union(idx + 1, idx + 2);
      }

      // also need to join cell with neighboring cells
      if (x > 0) {
        // join left of this cell with right of left cell
        uf.Union(idx + 3, (idx - 4) + 1);
      }
      if (y > 0) {
        // join top of this cell with bottom of above cell
        uf.Union(idx + 0, (idx - 4 * w) + 2);
      }
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  double time = timespec_diff(&end, &start);

  return time;
}

void TestUnionFind() {
  {
    UnionFind<uint32_t> uf(10);

    for (uint32_t i = 0; i < 10; i++) {
      assert(uf.Find(i) == i);
    }

    uf.Union(1, 3);
    uf.Union(4, 5);
    uf.Union(1, 5);

    assert(uf.Find(1) == uf.Find(3));
    assert(uf.Find(1) == uf.Find(4));
    assert(uf.Find(1) == uf.Find(5));
    assert(uf.Find(0) == 0);
    assert(uf.Find(2) == 2);
    assert(uf.Find(6) == 6);
    assert(uf.Find(7) == 7);
    assert(uf.Find(8) == 8);
    assert(uf.Find(9) == 9);
  }

#define NUM_TRIALS 10
  uint32_t widths[NUM_TRIALS] = { 1000, 1000, 1500, 2000, 2000,
                                  2000, 2500, 2500, 3000, 2500 };
  uint32_t heights[NUM_TRIALS] = { 1000, 2000, 2000, 2000, 2500,
                                   3000, 2800, 3200, 3000, 4000 };

  double times[NUM_TRIALS];

  for (int i = 0; i < NUM_TRIALS; i++) {
    times[i] = test_alg_1(widths[i], heights[i]);
    (void) times[i];
  }

  /*
printf("{");
for (int i = 0; i < NUM_TRIALS; i++) {
  printf("{%u,%f}", widths[i] * heights[i], times[i]);
  if (i != NUM_TRIALS - 1) {
      printf(",");
  }
}
printf("}\n");
  */
}
