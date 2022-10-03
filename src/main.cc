
#include <unistd.h>

#include "onoro.h"
#include "print_csi.h"

static constexpr uint32_t n_pawns = 8;

void TestUnionFind();

int main(int argc, char* argv[]) {
  srand(0);

  Onoro::Game<n_pawns> g;

  // printf("%s\n", g.Print().c_str());

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    g.forEachMove([&g](Onoro::idx_t idx) {
      Onoro::Game<n_pawns> g2(g, idx);
      g = std::move(g2);
      return false;
    });
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
      return -1;
    }

    int which = rand() % move_cnt;
    g.forEachMoveP2(
        [&g, &first, i, move_cnt, &which](Onoro::idx_t to, Onoro::idx_t from) {
          if (which == 0) {
            Onoro::Game<n_pawns> g2(g, to, from);

            if (first) {
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
    usleep(100);
  }

  // TestUnionFind();

  return 0;
}
