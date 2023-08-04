#include <unistd.h>
#include <utils/fun/print_csi.h>

#include "game.h"
#include "game_eq.h"
#include "game_hash.h"
#include "game_view.h"
#include "transposition_table.h"

static constexpr uint32_t n_pawns = 16;

static double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

void to_phase2(onoro::Game<n_pawns>& g) {
  while (!g.inPhase2()) {
    g.forEachMove([&g](onoro::P1Move move) {
      onoro::Game<n_pawns> g2(g, move);
      if (g2.isFinished()) {
        return true;
      }
      g = g2;
      return false;
    });
  }
}

uint64_t explore(const onoro::Game<n_pawns>& g, uint32_t depth) {
  uint64_t total_states = 1;

  if (g.isFinished() || g.inPhase2() || depth == 0) {
    return total_states;
  }

  g.forEachMove([&g, &total_states, depth](onoro::P1Move move) {
    onoro::Game<n_pawns> g2(g, move);
    total_states += explore(g2, depth - 1);
    return true;
  });

  return total_states;
}

uint64_t explore_p2(const onoro::Game<n_pawns>& g, uint32_t depth) {
  uint64_t total_states = 1;

  if (g.isFinished() || depth == 0) {
    return total_states;
  }

  g.forEachMoveP2([&g, &total_states, depth](onoro::P2Move move) {
    onoro::Game<n_pawns> g2(g, move);
    total_states += explore_p2(g2, depth - 1);
    return true;
  });

  return total_states;
}

int main() {
  onoro::Game<n_pawns> g;
  to_phase2(g);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  uint64_t n_states_explored = explore_p2(g, 5);
  clock_gettime(CLOCK_MONOTONIC, &end);

  printf("Explored %llu states in %f s\n", n_states_explored,
         timespec_diff(&start, &end));
  printf("%f states/sec\n", n_states_explored / timespec_diff(&start, &end));

  return 0;
}