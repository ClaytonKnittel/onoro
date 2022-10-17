
#include <absl/container/flat_hash_map.h>
#include <unistd.h>

#include "game.h"
#include "game_eq.h"
#include "game_hash.h"
#include "game_view.h"
#include "print_csi.h"

static constexpr uint32_t n_pawns = 16;
static uint64_t g_n_moves = 0;

using namespace onoro;
using namespace onoro::hash_group;

void TestUnionFind();

static double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

static int benchmark() {
  onoro::Game<n_pawns> g;

  static constexpr uint32_t n_moves = 6000000;

  for (uint32_t i = 0; i < n_pawns * 2 - 3; i++) {
    uint32_t move_cnt = 0;
    g.forEachMove([&move_cnt](onoro::idx_t idx) {
      move_cnt++;
      return true;
    });

    int which = rand() % move_cnt;
    g.forEachMove([&g, &which](onoro::idx_t idx) {
      if (which == 0) {
        onoro::Game<n_pawns> g2(g, idx);
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
    /*int move_cnt = 0;
    g.forEachMoveP2([&move_cnt](onoro::idx_t to, onoro::idx_t from) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      printf("Player won by no legal moves\n");
      return -1;
    }*/

    int which = 0;  // rand() % move_cnt;
    g.forEachMoveP2([&g, &which](onoro::idx_t to, onoro::idx_t from) {
      if (which == 0) {
        onoro::Game<n_pawns> g2(g, to, from);
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

class TranspositionTable {
  using TableT =
      absl::flat_hash_map<onoro::GameView<n_pawns>, int32_t,
                          onoro::GameHash<n_pawns>, onoro::GameEq<n_pawns>>;

 public:
  TranspositionTable() {}

  std::pair<TableT::iterator, bool> insert(onoro::Game<n_pawns>&& game) {
    // TODO: Try all symmetries of this game when looking up, if can't find,
    // then insert
  }

  const TableT& table() const {
    return table_;
  }

 private:
  TableT table_;
};

/*
 * Returns a chosen move along with the expected outcome, in terms of the
 * player to go. I.e., +1 = current player wins, 0 = tie, -1 = current player
 * loses.
 */
template <uint32_t NPawns>
static std::pair<int32_t, onoro::idx_t> findMove(const onoro::Game<NPawns>& g,
                                                 TranspositionTable& m,
                                                 int depth, int32_t alpha = -3,
                                                 int32_t beta = 3) {
  int32_t best_score = -2;
  onoro::idx_t best_move;

  if (!g.forEachMove([&g, &best_move, &best_score](onoro::idx_t idx) {
        onoro::Game<NPawns> g2(g, idx);
        if (g2.isFinished()) {
          best_score = 1;
          best_move = idx;
          return false;
        }
        return true;
      })) {
    return { best_score, best_move };
  }

  g.forEachMove(
      [&g, &m, &best_move, &best_score, depth, &alpha, beta](onoro::idx_t idx) {
        onoro::Game<NPawns> g2(g, idx);
        g_n_moves++;
        int32_t score;

        // If this move finished the game, it means playing it made us win.
        if (g2.isFinished()) {
          score = 1;
        } else {
          if (depth > 0) {
            auto [it, inserted] = m.insert(std::move(g2));
            if (!inserted) {
              score = it->second;
            } else {
              auto [_score, _] = findMove(g2, m, depth - 1, -beta, -alpha);
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

          if (best_score == 1 || best_score >= beta) {
            return false;
          }

          alpha = std::max(alpha, best_score);
        }
        return true;
      });

  return { best_score, best_move };
}

static int playout() {
  struct timespec start, end;
  onoro::Game<n_pawns> g;
  printf("%s\n", g.Print().c_str());

  TranspositionTable m;
  uint32_t max_depth = 2;

  for (uint32_t i = 0; i < n_pawns - 3; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);

    auto [score, move] = findMove(g, m, max_depth);
    // auto [score, move] = findMove(g, m, n_pawns - 4 - i);

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Move search time at depth %u: %lf s\n", timespec_diff(&start, &end),
           max_depth);

    if (score == -2) {
      printf("No moves available\n");
      break;
    }

    printf("Move (%d, %d), score %d (%llu playouts)\n", move.first, move.second,
           score, g_n_moves);
    g = onoro::Game<n_pawns>(g, move);
    printf("%s\n", g.Print().c_str());

    if (g.isFinished()) {
      printf("%s won\n", g.blackWins() ? "black" : "white");
      break;
    }

    break;
  }

  printf("Printing table contents:\n");
  for (auto it = m.table().cbegin(); it != m.table().cend(); it++) {
    printf("score: %d\n%s\n", it->second, it->first.game().Print().c_str());
  }

  return 0;
}

int main(int argc, char* argv[]) {
  static constexpr const uint32_t N = 16;

  printf("Game size: %zu bytes\n", sizeof(onoro::Game<N>));

  // return benchmark();
  return playout();
  onoro::GameHash<N> h;

  /*
  onoro::Game<N>::printSymmStateTableOps();
  printf("\n");
  onoro::Game<N>::printSymmStateTableSymms();

  onoro::Game<3>::printSymmStateTableOps();
  printf("\n");
  onoro::Game<3>::printSymmStateTableSymms();
  */

  if (!h.validate()) {
    printf("Invalid\n");
    return -1;
  } else {
    printf("Valid!\n");
  }

  onoro::Game<N> g1;
  g1.setTile((onoro::idx_t){ 8, 8 }, onoro::Game<N>::TileState::TILE_WHITE);
  g1.sum_of_mass_ += (onoro::HexPos){ 12, 8 };
  g1.state_.turn++;
  g1.state_.blackTurn = 1;

  onoro::Game<N> g2;
  g2.clearTile((onoro::idx_t){ 7, 7 });
  g2.clearTile((onoro::idx_t){ 7, 8 });
  g2.setTile((onoro::idx_t){ 7, 6 }, onoro::Game<N>::TileState::TILE_WHITE);
  g2.setTile((onoro::idx_t){ 7, 7 }, onoro::Game<N>::TileState::TILE_BLACK);
  g2.setTile((onoro::idx_t){ 6, 6 }, onoro::Game<N>::TileState::TILE_WHITE);
  g2.sum_of_mass_ += (onoro::HexPos){ 8, 4 };
  g2.state_.turn++;
  g2.state_.blackTurn = 1;

  if (!g1.validate() || !g2.validate()) {
    return -1;
  } else {
    printf("Valid states\n");
  }

  onoro::GameView<N> v1(&g1);
  onoro::GameView<N> v2(&g2, K4(C2(0), C2(0)), true);

  printf("%s\n", v1.game().Print().c_str());
  printf("%s\n", v1.game().Print2().c_str());
  printf("%s\n", v2.game().Print().c_str());
  printf("%s\n", v2.game().Print2().c_str());
  printf("Hash 1: %s\n",
         onoro::GameHash<N>::printK4Hash(v1.template hash<K4>()).c_str());
  printf("Hash 2: %s\n",
         onoro::GameHash<N>::printK4Hash(v2.template hash<K4>()).c_str());

  GameEq<N> eq;
  printf("Are equal? %s\n", eq(v1, v2) ? "true" : "false");
  printf("Are equal? %s\n", eq(v2, v1) ? "true" : "false");

  return 0;
}
