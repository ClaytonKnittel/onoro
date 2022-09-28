
#include "onoro.h"

static constexpr uint32_t n_pawns = 4;

int main(int argc, char* argv[]) {
  Onoro::Game<n_pawns> g;

  printf("%s\n", g.Print().c_str());

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    g.forEachMove([&g](Onoro::idx_t idx) {
      Onoro::Game<n_pawns> g2(g, idx);
      printf("%s\n", g2.Print().c_str());

      g = std::move(g2);
      return false;
    });
  }

  g.forEachMoveP2([&g](Onoro::idx_t to, Onoro::idx_t from) {
    printf("Move from (%u, %u) to (%u, %u)\n", from.first, from.second,
           to.first, to.second);
    Onoro::Game<n_pawns> g2(g, to, from);
    printf("%s\n", g2.Print().c_str());
  });

  return 0;
}
