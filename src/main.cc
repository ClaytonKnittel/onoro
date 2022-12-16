
#include <absl/types/optional.h>
#include <unistd.h>
#include <utils/fun/print_csi.h>

#include <cinttypes>
#include <queue>

#include "game.h"
#include "game_eq.h"
#include "game_hash.h"
#include "game_view.h"
#include "transposition_table.h"

using namespace onoro;
using namespace onoro::hash_group;

template <uint32_t NPawns>
static uint64_t count_board_states() {
  onoro::Game<NPawns> starting_g;
  onoro::TranspositionTable<NPawns> m;
  std::queue<onoro::Game<NPawns>> frontier;

  frontier.push(starting_g);
  m.insert(starting_g);

  while (!frontier.empty()) {
    onoro::Game<NPawns> g = frontier.front();
    frontier.pop();

    if (g.inPhase2()) {
      g.forEachMoveP2([&g, &m, &frontier](onoro::P2Move move) {
        onoro::Game<NPawns> g2(g, move);

        if (!m.find(g2).has_value()) {
          frontier.push(g2);
          m.insert(g2);
        }
        return true;
      });
    } else {
      g.forEachMove([&g, &m, &frontier](onoro::P1Move move) {
        onoro::Game<NPawns> g2(g, move);

        if (!m.find(g2).has_value()) {
          frontier.push(g2);
          m.insert(g2);
        }
        return true;
      });
    }
  }

  uint64_t cnt = 0;
  for (const onoro::Game<NPawns>& g : m.table()) {
    if (g.nPawnsInPlay() == NPawns) {
      // printf("%s\n", g.Print().c_str());
      cnt++;
    }
  }

  return cnt;
}

template <uint32_t NPawns>
static void count_all() {
  count_all<NPawns - 1>();
  printf("N game states of size %u: %" PRIu64 "\n", NPawns,
         count_board_states<NPawns>());
}

template <>
void count_all<2>() {}

int main(int argc, char* argv[]) {
  static constexpr const uint32_t N = 16;

  count_all<N>();
}
