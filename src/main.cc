
#include "onoro.h"

static constexpr uint32_t n_pawns = 4;

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

  for (uint32_t i = 0; i < 15; i++) {
    int move_cnt = 0;
    g.forEachMoveP2([&move_cnt](Onoro::idx_t to, Onoro::idx_t from) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      return -1;
    }

    int which = rand() % move_cnt;
    g.forEachMoveP2([&g, &which](Onoro::idx_t to, Onoro::idx_t from) {
      if (which == 0) {
        printf("Move from (%u, %u) to (%u, %u)\n", from.first, from.second,
               to.first, to.second);
        Onoro::Game<n_pawns> g2(g, to, from);
        g = std::move(g2);
        return false;
      } else {
        which--;
        return true;
      }
    });

    printf("%s\n", g.Print().c_str());
  }

  return 0;
}
