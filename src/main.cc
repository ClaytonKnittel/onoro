
#include <absl/container/flat_hash_map.h>
#include <unistd.h>
#include <utils/fun/print_csi.h>

#include "game.h"
#include "game_eq.h"
#include "game_hash.h"
#include "game_view.h"

static constexpr uint32_t n_pawns = 16;
static uint64_t g_n_moves = 0;
static uint64_t g_n_misses = 0;
static uint64_t g_n_hits = 0;

using namespace onoro;
using namespace onoro::hash_group;

void TestUnionFind();

static double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

static int benchmark() {
  onoro::Game<n_pawns> g;

  static constexpr uint32_t n_moves = 600000;

  for (uint32_t i = 0; i < n_pawns - 3; i++) {
    uint32_t move_cnt = 0;
    g.forEachMove([&move_cnt](onoro::P1Move move) {
      move_cnt++;
      return true;
    });

    int which = rand() % move_cnt;
    g.forEachMove([&g, &which](onoro::P1Move move) {
      if (which == 0) {
        onoro::Game<n_pawns> g2(g, move);
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
    g.forEachMoveP2([&g, &which](onoro::P2Move move) {
      if (which == 0) {
        onoro::Game<n_pawns> g2(g, move);
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
  using SymmState = typename Game<n_pawns>::BoardSymmetryState;

 public:
  TranspositionTable() {}

  TableT::const_iterator find(const onoro::GameView<n_pawns>& view) {
    SymmState s = view.game().calcSymmetryState();
    SymmetryClassOpApplyAndReturn(s.symm_class, tryFindSymmetries, view, s);
  }

  void insert(onoro::GameView<n_pawns>&& view, int32_t score) {
    table_.emplace(view, score);
  }

  const TableT& table() const {
    return table_;
  }

  const TableT::const_iterator end() const {
    return table_.end();
  }

 private:
  template <class SymmetryClassOp>
  TableT::const_iterator tryFindSymmetries(onoro::GameView<n_pawns> view,
                                           SymmState symm_state) {
    typedef typename SymmetryClassOp::Group Group;

    // printf("%s\n", view.game().Print().c_str());

    for (bool swap_colors : { false, true }) {
      (void) swap_colors;

      for (uint32_t op_ord = 0; op_ord < Group::order(); op_ord++) {
        Group op(op_ord);
        view.setOp(op);
        // printf("hash: %s\n",
        //        GameHash<n_pawns>::printC2Hash(view.hash()).c_str());

        auto it = table_.find(view);
        if (it != end()) {
          /*printf("Found under %s (%s) (%s)!\n", op.toString().c_str(),
                 swap_colors ? "swapped" : "not swapped",
                 symm_state.op.toString().c_str());
          printf("\n");*/
          return it;
          /*} else {
            printf("Didn't find under %s (%s) (%s)!\n", op.toString().c_str(),
                   swap_colors ? "swapped" : "not swapped",
                   symm_state.op.toString().c_str());*/
        }
      }

      view.invertColors();
    }
    // printf("\n");

    return end();
  }

 private:
  TableT table_;
};

/*
 * Returns a chosen move along with the expected outcome, in terms of the
 * player to go. I.e., +1 = current player wins, 0 = tie, -1 = current player
 * loses.
 */
template <uint32_t NPawns, class MoveClass>
static std::pair<int32_t, MoveClass> findMove(const onoro::Game<NPawns>& g,
                                              TranspositionTable& m, int depth,
                                              int32_t alpha = -3,
                                              int32_t beta = 3) {
  int32_t best_score = -2;
  MoveClass best_move;

  if (!MoveClass::forEachMoveFn(g,
                                [&g, &best_move, &best_score](MoveClass move) {
                                  onoro::Game<NPawns> g2(g, move);
                                  if (g2.isFinished()) {
                                    best_score = 1;
                                    best_move = move;
                                    return false;
                                  }
                                  return true;
                                })) {
    return { best_score, best_move };
  }

  MoveClass::forEachMoveFn(g, [&g, &m, &best_move, &best_score, depth, &alpha,
                               beta](MoveClass move) {
    onoro::Game<NPawns> g2(g, move);
    g_n_moves++;
    int32_t score;

    // If this move finished the game, it means playing it made us win.
    if (g2.isFinished()) {
      score = 1;
    } else {
      if (depth > 0) {
        onoro::Game<n_pawns>* g_ptr = new onoro::Game<n_pawns>(std::move(g2));
        onoro::GameView<n_pawns> view(g_ptr);
        auto it = m.find(view);

        if (it != m.end()) {
          g_n_hits++;

          score =
              it->second *
              ((it->first.areColorsInverted() ^ view.areColorsInverted()) ? -1
                                                                          : 1);
        } else {
          g_n_misses++;

          int32_t _score;
          if (std::is_same<MoveClass, onoro::P2Move>::value || g2.inPhase2()) {
            _score =
                findMove<NPawns, onoro::P2Move>(g2, m, depth - 1, -beta, -alpha)
                    .first;
          } else {
            _score =
                findMove<NPawns, onoro::P1Move>(g2, m, depth - 1, -beta, -alpha)
                    .first;
          }
          score = std::min(-_score, 1);

          m.insert(std::move(view), score);
        }
      } else {
        score = 0;
      }
    }

    if (score > best_score) {
      best_move = move;
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
  uint32_t max_depth = 20;

  // for (uint32_t i = 0; i < n_pawns - 3; i++) {
  for (uint32_t i = 0; i < 1; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);

    auto [score, move] = findMove<n_pawns, onoro::P1Move>(g, m, max_depth);
    // auto [score, move] = findMove(g, m, n_pawns - 4 - i);

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Move search time at depth %u: %lf s\n", timespec_diff(&start, &end),
           max_depth);

    if (score == -2) {
      printf("No moves available\n");
      break;
    }

    printf("Move (%d, %d), score %d (%llu playouts, %f%% hits)\n",
           move.loc.first, move.loc.second, score, g_n_moves,
           100. * g_n_hits / (double) (g_n_misses + g_n_hits));
    g_n_misses = 0;
    g_n_hits = 0;

    g = onoro::Game<n_pawns>(g, move);
    printf("%s\n", g.Print().c_str());

    if (g.isFinished()) {
      printf("%s won\n", g.blackWins() ? "black" : "white");
      break;
    }

    break;
  }

  /*
  printf("Printing table contents:\n");
  uint32_t cnt = 0;
  for (auto it = m.table().cbegin(); it != m.table().cend(); it++) {
    printf("score: %d, n_pawns: %u\n%s\n", it->second,
           it->first.game().nPawnsInPlay(), it->first.game().Print().c_str());

    if (cnt++ == 32) {
      break;
    }
  }
  */
  printf("Table size: %zu\n", m.table().size());

  return 0;
}

int main(int argc, char* argv[]) {
  static constexpr const uint32_t N = 8;

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
  printf("Hash 1: %s\n", onoro::GameHash<N>::printK4Hash(v1.hash()).c_str());
  printf("Hash 2: %s\n", onoro::GameHash<N>::printK4Hash(v2.hash()).c_str());

  GameEq<N> eq;
  printf("Are equal? %s\n", eq(v1, v2) ? "true" : "false");
  printf("Are equal? %s\n", eq(v2, v1) ? "true" : "false");

  return 0;
}
