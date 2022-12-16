
#include <unistd.h>

#include "onoro.h"
#include "transposition_table.h"

static constexpr uint32_t n_pawns = 16;
static uint64_t g_n_moves = 0;

static double timespec_diff(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) +
         (((double) (end->tv_nsec - start->tv_nsec)) / 1000000000.);
}

template <class SymmetryClassOp>
bool eqUnderSymmT(const onoro::Game<n_pawns>& game1,
                  const onoro::Game<n_pawns>& game2) {
  typedef typename SymmetryClassOp::Group Group;

  onoro::GameView<n_pawns> view1(&game1);

  onoro::GameEq<n_pawns> games_eq;

  for (bool swap_colors : { false, true }) {
    (void) swap_colors;

    for (uint32_t op_ord = 0; op_ord < Group::order(); op_ord++) {
      Group op(op_ord);
      view1.setOp(op);

      if (games_eq(view1, game2)) {
        return true;
      }
    }

    view1.invertColors();
  }

  return false;
}

bool eqUnderSymm(const onoro::Game<n_pawns>& game1,
                 const onoro::Game<n_pawns>& game2) {
  onoro::Game<n_pawns>::BoardSymmetryState s = game1.calcSymmetryState();
  SymmetryClassOpApplyAndReturn(s.symm_class, eqUnderSymmT, game1, game2);
}

/*
 * Returns a chosen move along with the expected outcome, in terms of the
 * player to go. I.e., +1 = current player wins, 0 = tie, -1 = current player
 * loses.
 */
template <uint32_t NPawns, class MoveClass>
static std::pair<int32_t, MoveClass> findMove(
    const onoro::Game<NPawns>& g, onoro::TranspositionTable<NPawns>& m,
    int depth) {
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

  MoveClass::forEachMoveFn(g, [&g, &m, &best_move, &best_score,
                               depth](MoveClass move) {
    onoro::Game<NPawns> g2(g, move);
    g_n_moves++;
    int32_t score;

    // If this move finished the game, it means playing it made us win.
    if (g2.isFinished()) {
      score = 1;
    } else {
      if (depth > 0) {
        auto cached_score = m.find(g2);

        if (!cached_score.has_value()) {
          m.insert(g2);
        }

        int32_t _score;
        if (std::is_same<MoveClass, onoro::P2Move>::value || g2.inPhase2()) {
          _score = findMove<NPawns, onoro::P2Move>(g2, m, depth - 1).first;
        } else {
          _score = findMove<NPawns, onoro::P1Move>(g2, m, depth - 1).first;
        }
        score = std::min(-_score, 1);

        if (cached_score.has_value()) {
          if (score != *cached_score) {
            printf("Turn: %s\n%s\n", g2.blackTurn() ? "black" : "white",
                   g2.Print().c_str());
            printf("Expected score %d, but found %d in hash table\n", score,
                   *cached_score);
            onoro::proto::GameState gs = g2.SerializeState();
            gs.SerializeToOstream(&std::cerr);
            abort();
          }
        } else {
          g2.setScore(score);
          m.insert_or_assign(std::move(g2));
        }
      } else {
        score = 0;
      }
    }

    if (score > best_score) {
      best_move = move;
      best_score = score;

      if (best_score == 1) {
        return false;
      }
    }
    return true;
  });

  return { best_score, best_move };
}

static int test_transposition_table() {
  struct timespec start, end;
  onoro::Game<n_pawns> g;

  printf("%s\n", g.Print().c_str());

  onoro::TranspositionTable<n_pawns> m;
  uint32_t moves_in_phase_1 = 13;

  for (uint32_t i = 0; i < moves_in_phase_1; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    int32_t score;
    onoro::P1Move p1_move;
    onoro::P2Move p2_move;

    m.clear();

    uint32_t max_depth = std::min(moves_in_phase_1 - i, 9u);

    if (g.inPhase2()) {
      auto [_score, move] = findMove<n_pawns, onoro::P2Move>(g, m, max_depth);
      score = _score;
      p2_move = move;
    } else {
      auto [_score, move] = findMove<n_pawns, onoro::P1Move>(g, m, max_depth);
      score = _score;
      p1_move = move;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Move search time at depth %u: %lf s\n", timespec_diff(&start, &end),
           max_depth);

    if (score == -2) {
      printf("No moves available\n");
      break;
    }

    if (g.inPhase2()) {
      onoro::idx_t from = g.idxAt(p2_move.from_idx);
      printf(
          "Move (%d, %d) from (%d, %d), score %d (%llu playouts, %f "
          "playouts/sec)\n",
          p2_move.to.x(), p2_move.to.y(), from.x(), from.y(), score, g_n_moves,
          (double) g_n_moves / timespec_diff(&start, &end));
    } else {
      printf("Move (%d, %d), score %d (%llu playouts, %f playouts/sec)\n",
             p1_move.loc.x(), p1_move.loc.y(), score, g_n_moves,
             (double) g_n_moves / timespec_diff(&start, &end));
    }
    g_n_moves = 0;

    if (g.inPhase2()) {
      g = onoro::Game<n_pawns>(g, p2_move);
    } else {
      g = onoro::Game<n_pawns>(g, p1_move);
    }
    printf("%s\n", g.Print().c_str());

    if (g.isFinished()) {
      printf("%s won\n", g.blackWins() ? "black" : "white");
      break;
    }
  }

  return 0;
}

int main(int argc, char* argv[]) {
  return test_transposition_table();
}
