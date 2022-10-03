
#include <unistd.h>

#include "onoro.h"
#include "print_csi.h"

static constexpr uint32_t n_pawns = 8;
static constexpr bool print_overwrite = false;

void TestUnionFind();

static double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

static int benchmark() {
  Onoro::Game<n_pawns> g;

  static constexpr uint32_t n_moves = 60000;

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    uint32_t move_cnt = 0;
    g.forEachMove([&move_cnt](Onoro::idx_t idx) {
      move_cnt++;
      return true;
    });

    int which = rand() % move_cnt;
    g.forEachMove([&g, &which](Onoro::idx_t idx) {
      if (which == 0) {
        Onoro::Game<n_pawns> g2(g, idx);
        g = std::move(g2);
        return false;
      } else {
        which--;
        return true;
      }
    });
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (uint32_t i = 0; i < n_moves; i++) {
    int move_cnt = 0;
    g.forEachMoveP2([&move_cnt](Onoro::idx_t to, Onoro::idx_t from) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      printf("Player won by no legal moves\n");
      return -1;
    }

    int which = rand() % move_cnt;
    g.forEachMoveP2([&g, &which](Onoro::idx_t to, Onoro::idx_t from) {
      if (which == 0) {
        Onoro::Game<n_pawns> g2(g, to, from);
        g = std::move(g2);
        return false;
      } else {
        which--;
        return true;
      }
    });
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  printf("Did %u moves in %f s\n", n_moves, timespec_diff(&start, &end));

  return 0;
}

int main(int argc, char* argv[]) {
  srand(0);

  return benchmark();

  Onoro::Game<n_pawns> g;

  // printf("%s\n", g.Print().c_str());

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    uint32_t move_cnt = 0;
    g.forEachMove([&move_cnt](Onoro::idx_t idx) {
      move_cnt++;
      return true;
    });

    int which = rand() % move_cnt;
    g.forEachMove([&g, &which](Onoro::idx_t idx) {
      if (which == 0) {
        Onoro::Game<n_pawns> g2(g, idx);
        g = std::move(g2);
        return false;
      } else {
        which--;
        return true;
      }
    });

    if (g.isFinished()) {
      printf("%s\n", g.Print().c_str());
      return 0;
    }
  }

  printf("%s\n", g.Print().c_str());

  bool first = true;
  for (uint32_t i = 0; i < 30000; i++) {
    int move_cnt = 0;
    g.forEachMoveP2([&move_cnt](Onoro::idx_t to, Onoro::idx_t from) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      printf("Player won by no legal moves\n");
      return -1;
    }

    int which = rand() % move_cnt;
    g.forEachMoveP2(
        [&g, &first, i, move_cnt, &which](Onoro::idx_t to, Onoro::idx_t from) {
          if (which == 0) {
            Onoro::Game<n_pawns> g2(g, to, from);

            if (first || !print_overwrite) {
              first = false;
            } else {
              std::ostringstream ostr;
              ostr << CSI_CHA(0) CSI_EL(CSI_CURSOR_ALL);
              for (uint32_t j = 0; j < 2 * n_pawns + 3; j++) {
                ostr << CSI_CUU(1) CSI_EL(CSI_CURSOR_ALL);
              }

              printf("%s", ostr.str().c_str());
            }
            printf("Move from (%u, %u) to (%u, %u), of (%u) (%u)\n", from.first,
                   from.second, to.first, to.second, move_cnt, i);
            g = std::move(g2);
            return false;
          } else {
            which--;
            return true;
          }
        });

    printf("%s\n", g.Print().c_str());
    if (g.isFinished()) {
      printf("Done!\n");
      return 0;
    }
    usleep(100);
  }

  // TestUnionFind();

  return 0;
}
