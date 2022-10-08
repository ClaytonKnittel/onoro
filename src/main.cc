
#include <absl/container/flat_hash_map.h>
#include <unistd.h>

#include "dihedral_group.h"
#include "game_hash.h"
#include "onoro.h"
#include "print_csi.h"

static constexpr uint32_t n_pawns = 3;

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

typedef absl::flat_hash_map<Onoro::Game<n_pawns>, int32_t,
                            Onoro::GameHash<n_pawns>, Onoro::GameEq<n_pawns>>
    TranspositionTable;

/*
 * Returns a chosen move along with the expected outcome, in terms of the
 * player to go. I.e., +1 = current player wins, 0 = tie, -1 = current player
 * loses.
 */
template <uint32_t NPawns>
static std::pair<int32_t, Onoro::idx_t> findMove(const Onoro::Game<NPawns>& g,
                                                 TranspositionTable& m,
                                                 int depth) {
  int32_t best_score = -2;
  Onoro::idx_t best_move;

  g.forEachMove([&g, &m, &best_move, &best_score, depth](Onoro::idx_t idx) {
    Onoro::Game<NPawns> g2(g, idx);
    int32_t score;

    // If this move finished the game, it means playing it made us win.
    if (g2.isFinished()) {
      score = 1;
    } else {
      if (depth > 0) {
        auto [it, inserted] = m.insert(std::make_pair(std::move(g2), 0));
        if (!inserted) {
          score = it->second;
        } else {
          auto [_score, _] = findMove(g2, m, depth - 1);
          score = std::min(-_score, 1);

          it->second = score;
        }
      } else {
        score = 0;
      }
    }

    if (score > best_score) {
      best_move = idx;
      best_score = score;

      if (best_score == 1) {
        return false;
      }
    }
    return true;
  });

  return { best_score, best_move };
}

static int playout() {
  Onoro::Game<n_pawns> g;
  printf("%s\n", g.Print().c_str());

  TranspositionTable m;

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    auto [score, move] = findMove(g, m, std::min(1000u, n_pawns * 2 - 4 - i));

    if (score == -2) {
      printf("No moves available\n");
      break;
    }

    printf("Move (%u, %u), score %d\n", move.first, move.second, score);
    g = Onoro::Game<n_pawns>(g, move);
    printf("%s\n", g.Print().c_str());

    if (g.isFinished()) {
      printf("%s won\n", g.blackWins() ? "black" : "white");
      break;
    }
  }

  printf("Printing table contents:\n");
  for (auto it = m.cbegin(); it != m.cend(); it++) {
    printf("score: %d\n%s\n", it->second, it->first.Print().c_str());
  }

  return 0;
}

int main(int argc, char* argv[]) {
  srand(0);

  // return benchmark();
  // return playout();
  Onoro::GameHash<0> h;
  if (!h.validate()) {
    printf("Invalid\n");
  } else {
    printf("Valid!\n");
  }

  /*
  typedef DihedralEl<6> D6;
  D6 r1(D6::Action::ROT, 1);
  D6 r5(D6::Action::ROT, 5);
  D6 s0(D6::Action::REFL, 0);
  D6 rot = r1 * (s0 * r5);
  std::cout << rot.toString() << std::endl;
  */

  return 0;
}
