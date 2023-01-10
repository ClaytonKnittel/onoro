
#include <unistd.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "onoro.h"
#include "transposition_table.h"

static constexpr uint32_t n_pawns = 8;
static uint64_t g_n_moves = 0;

ABSL_FLAG(uint32_t, depth, 8, "Search depth to test to");
ABSL_FLAG(bool, from_stdin, false,
          "If set, reads a game state proto from stdin.");

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
/*
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
          if (score != cached_score->score()) {
            printf("Turn: %s\n%s\n", g2.blackTurn() ? "black" : "white",
                   g2.Print().c_str());
            printf("Expected score %d, but found %d in hash table\n", score,
                   cached_score->score());
            onoro::proto::GameState gs = g2.SerializeState();
            gs.SerializeToOstream(&std::cerr);
            abort();
          }
        } else {
          g2.setScore(onoro::Score(score, 0));
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
*/

onoro::Game<n_pawns> g_game;

template <uint32_t NPawns, class MoveClass>
static std::pair<absl::optional<onoro::Score>, MoveClass> findMove(
    const onoro::Game<NPawns>& g, onoro::TranspositionTable<NPawns>& m,
    uint32_t depth) {
  absl::optional<onoro::Score> best_score;
  MoveClass best_move;

  if (depth == 0) {
    return { onoro::Score::tie(0), MoveClass() };
  }

  if (!MoveClass::forEachMoveFn(g,
                                [&g, &best_move, &best_score](MoveClass move) {
                                  onoro::Game<NPawns> g2(g, move);
                                  if (g2.isFinished()) {
                                    best_score = onoro::Score::win(1);
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
    onoro::Score score;

    // If this move finished the game, it means playing it made us win.
    if (g2.isFinished()) {
      score = onoro::Score::win(1);
    } else {
      auto cached_score = m.find(g2);

      // if (cached_score.has_value() && cached_score->determined(depth)) {
      //   score = *cached_score;
      // } else if (cached_score.has_value() &&
      //            *cached_score == onoro::Score::ancestor()) {
      //   score = onoro::Score::tie(1);
      // }

      // g2.setScore(onoro::Score::ancestor());
      // m.insert(g2);

      absl::optional<onoro::Score> _score;
      if (std::is_same<MoveClass, onoro::P2Move>::value || g2.inPhase2()) {
        _score = findMove<NPawns, onoro::P2Move>(g2, m, depth - 1).first;
      } else {
        _score = findMove<NPawns, onoro::P1Move>(g2, m, depth - 1).first;
      }
      if (!_score.has_value()) {
        // Consider winning by no legal moves as not winning until after the
        // other player's attempt at making a move, since all game states that
        // don't have 4 in a row of a pawn are considered a tie.
        score = onoro::Score::win(2);
      } else {
        score = _score->backstep();
      }

      if (cached_score.has_value()) {
        if (!cached_score->compatible(score)) {
          // if (cached_score->determined(depth)) {
          //   if (cached_score->score(depth) != score.score(depth)) {
          printf("depth: %u\n", depth);
          printf(
              "%s\nIncompatible scores found at depth %u: cache %s, vs. "
              "calc %s\n",
              g2.Print().c_str(), depth, cached_score->Print().c_str(),
              score.Print().c_str());
          onoro::proto::GameState gs = g2.SerializeState();
          gs.SerializeToOstream(&std::cerr);
          abort();
          //   }
          // }
        }
      }

      onoro::Score merged_score =
          cached_score.has_value() ? cached_score->merge(score) : score;
      g2.setScore(merged_score);
      m.insert_or_assign(std::move(g2));

      if (eqUnderSymm(g2, g_game)) {
        printf("depth: %u\n", depth);
        printf("%s\n", g2.Print().c_str());
        printf("%s (%s + %s)\n", merged_score.Print().c_str(),
               cached_score.has_value() ? cached_score->Print().c_str() : "[]",
               score.Print().c_str());
      }
    }

    if (!best_score.has_value()) {
      best_score = score;
      best_move = move;
    } else if (score.better(*best_score)) {
      best_move = move;
      best_score = score;

      if (score.score(depth) == 1) {
        // We can stop the search early if we already have a winning move.
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

  onoro::TranspositionTable<n_pawns> m;
  uint32_t max_depth = absl::GetFlag(FLAGS_depth);
  uint32_t max_turns = 1;

  if (absl::GetFlag(FLAGS_from_stdin)) {
    onoro::proto::GameState gs;
    gs.ParseFromIstream(&std::cin);
    const auto err = onoro::Game<n_pawns>::LoadState(gs);
    if (!err.ok()) {
      printf("Failed to parse: %s\n", err.status().message());
      return -1;
    }
    g_game = *err;
    // g = g_game;
  }

  printf("%s\n", g.Print().c_str());

  for (uint32_t i = 0; i < max_turns; i++) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    absl::optional<onoro::Score> score;
    onoro::P1Move p1_move;
    onoro::P2Move p2_move;

    m.clear();

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

    if (!score.has_value()) {
      printf("No moves available\n");
      break;
    }

    if (g.inPhase2()) {
      onoro::idx_t from = g.idxAt(p2_move.from_idx);
      printf(
          "Move (%d, %d) from (%d, %d), score %s (%llu playouts, %f "
          "playouts/sec)\n",
          p2_move.to.x(), p2_move.to.y(), from.x(), from.y(),
          score->Print().c_str(), g_n_moves,
          (double) g_n_moves / timespec_diff(&start, &end));
    } else {
      printf("Move (%d, %d), score %s (%llu playouts, %f playouts/sec)\n",
             p1_move.loc.x(), p1_move.loc.y(), score->Print().c_str(),
             g_n_moves, (double) g_n_moves / timespec_diff(&start, &end));
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
  absl::ParseCommandLine(argc, argv);

  return test_transposition_table();
}
