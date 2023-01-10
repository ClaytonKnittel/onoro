
#include <absl/types/optional.h>
#include <unistd.h>
#include <utils/fun/print_csi.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "game.h"
#include "game_eq.h"
#include "game_hash.h"
#include "game_view.h"
#include "transposition_table.h"

ABSL_FLAG(uint32_t, depth, 8, "Search depth to test to");
ABSL_FLAG(bool, from_stdin, false,
          "If set, reads a game state proto from stdin.");

template <uint32_t NPawns, typename Hash>
bool onoro::Game<NPawns, Hash>::operator==(
    const onoro::Game<NPawns, Hash>& other) const {
  onoro::GameEq<NPawns> eq;
  return eq(*this, other);
}

static constexpr uint32_t n_pawns = 12;
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

template <uint32_t NPawns>
bool verifySerializesToSelf(const onoro::Game<n_pawns>& g) {
  auto s = g.SerializeState();
  // printf("%s\n", s.DebugString().c_str());

  absl::StatusOr<onoro::Game<NPawns>> res = onoro::Game<NPawns>::LoadState(s);
  if (!res.ok()) {
    std::cout << res.status() << std::endl;
    return false;
  }
  onoro::Game<NPawns> g2 = *res;

  onoro::GameView<NPawns> v1(&g);
  onoro::GameView<NPawns> v2(&g2);
  GameEq<NPawns> eq;

  return eq(v1, v2);
}

static int benchmark() {
  srand(0);
  onoro::Game<n_pawns> g;

  static constexpr uint32_t n_moves = 600000;

  for (uint32_t i = 0; i < n_pawns - 3; i++) {
    uint32_t move_cnt = 0;
    g.forEachMove([&move_cnt](onoro::P1Move move) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      printf("no legal moves!\n");
      return -1;
    }

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

    if (g.isFinished()) {
      printf("%s\n", g.Print().c_str());
      printf("%s won!\n", g.blackWins() ? "black" : "white");
      return 0;
    }
  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  uint32_t i;
  for (i = 0; i < n_moves; i++) {
    int move_cnt = 0;
    g.forEachMoveP2([&move_cnt](onoro::P2Move move) {
      move_cnt++;
      return true;
    });

    if (move_cnt == 0) {
      printf("Player won by no legal moves\n");
      printf("%s\n", g.Print().c_str());
      break;
    }

    int which = rand() % move_cnt;
    g.forEachMoveP2([&g, &which](onoro::P2Move move) {
      if (which == 0) {
        onoro::Game<n_pawns> g2(g, move);
        g = std::move(g2);

        /*
        std::ostringstream ostr;
        if (i != 0) {
          ostr << CSI_CHA(0);
          for (uint32_t j = 0; j < n_pawns; j++) {
            ostr << CSI_CUU(1) CSI_EL(CSI_CURSOR_ALL);
          }
        }
        printf("%s%s\n", ostr.str().c_str(), g.Print().c_str());
        usleep(1000);
        */
        return false;
      } else {
        which--;
        return true;
      }
    });
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  printf("Did %u moves in %f s\n", i, timespec_diff(&start, &end));
  printf("%f moves/sec\n", i / timespec_diff(&start, &end));

  return 0;
}

/*
 * Returns a chosen move along with the expected outcome, in terms of the
 * player to go. I.e., +1 = current player wins, 0 = tie, -1 = current player
 * loses.
 */
template <uint32_t NPawns, class MoveClass>
static std::pair<int32_t, MoveClass> findMoveAB(const onoro::Game<NPawns>& g,
                                                int depth, int32_t alpha = -3,
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

  MoveClass::forEachMoveFn(g, [&g, &best_move, &best_score, depth, &alpha,
                               beta](MoveClass move) {
    onoro::Game<NPawns> g2(g, move);
    g_n_moves++;
    int32_t score;

    // If this move finished the game, it means playing it made us win.
    if (g2.isFinished()) {
      score = 1;
    } else {
      if (depth > 0) {
        int32_t _score;
        if (std::is_same<MoveClass, onoro::P2Move>::value || g2.inPhase2()) {
          _score =
              findMoveAB<NPawns, onoro::P2Move>(g2, depth - 1, -beta, -alpha)
                  .first;
        } else {
          _score =
              findMoveAB<NPawns, onoro::P1Move>(g2, depth - 1, -beta, -alpha)
                  .first;
        }
        score = std::min(-_score, 1);
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

      if (cached_score.has_value() && cached_score->determined(depth)) {
        score = *cached_score;
        g_n_hits++;
      } else {
        g_n_misses++;

        absl::optional<onoro::Score> _score;
        if (std::is_same<MoveClass, onoro::P2Move>::value || g2.inPhase2()) {
          _score = findMove<NPawns, onoro::P2Move>(g2, m, depth - 1).first;
        } else {
          _score = findMove<NPawns, onoro::P1Move>(g2, m, depth - 1).first;
        }
        if (!_score.has_value()) {
          // Consider winning by no legal moves as not winning until after the
          // other player's attempt at making a move, since all game states
          // that don't have 4 in a row of a pawn are considered a tie.
          score = onoro::Score::win(2);
        } else {
          score = _score->backstep();
        }

        // Update the cached score in case it changed.
        cached_score = m.find(g2);

        onoro::Score merged_score =
            cached_score.has_value() ? cached_score->merge(score) : score;
        g2.setScore(merged_score);
        m.insert_or_assign(std::move(g2));
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

static void allCompatible(const TranspositionTable<n_pawns>& t1,
                          const TranspositionTable<n_pawns>& t2) {
  for (auto it = t1.table().cbegin(); it != t1.table().cend(); it++) {
    const auto s1 = it->getScore();

    const auto s2 = t2.find(*it);
    if (s2.has_value()) {
      if (!s1.compatible(*s2)) {
        printf("%s\n", it->Print().c_str());
        printf("Incompatible scores: t1 has %s, t2 has %s\n", s1, *s2);
        abort();
      }
    }
  }
}

static int playout() {
  struct timespec start, end;
  onoro::Game<n_pawns> g;
  onoro::Game<n_pawns> prev;

  printf("Game size: %zu bytes\n", sizeof(onoro::Game<n_pawns>));
  printf("Game view size: %zu bytes\n", sizeof(onoro::GameView<n_pawns>));

  printf("%s\n", g.Print().c_str());
  prev = g;

  TranspositionTable<n_pawns> m;
  uint32_t max_depth = absl::GetFlag(FLAGS_depth);
  std::vector<onoro::Game<n_pawns>> history;

  for (uint32_t i = 0; i < -1u; i++) {
    if (std::find(history.cbegin(), history.cend(), prev) != history.cend()) {
      printf("State has been repeated!\n");
      break;
    }
    history.push_back(prev);

    clock_gettime(CLOCK_MONOTONIC, &start);
    absl::optional<onoro::Score> score;
    P1Move p1_move;
    P2Move p2_move;

    // m.clear();

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
    printf("Move search time at depth %u: %lf s (table size: %zu)\n",
           timespec_diff(&start, &end), max_depth, m.table().size());

    if (!score.has_value()) {
      printf("No moves available\n");
      break;
    }

    if (g.inPhase2()) {
      onoro::idx_t from = g.idxAt(p2_move.from_idx);
      printf(
          "Move (%d, %d) from (%d, %d), %s (%llu playouts, %f%% hits, %f "
          "playouts/sec)\n",
          p2_move.to.x(), p2_move.to.y(), from.x(), from.y(),
          score->Print().c_str(), g_n_moves,
          100. * g_n_hits / (double) (g_n_misses + g_n_hits),
          (double) g_n_moves / timespec_diff(&start, &end));
    } else {
      printf("Move (%d, %d), %s (%llu playouts, %f%% hits, %f playouts/sec)\n",
             p1_move.loc.x(), p1_move.loc.y(), score->Print().c_str(),
             g_n_moves, 100. * g_n_hits / (double) (g_n_misses + g_n_hits),
             (double) g_n_moves / timespec_diff(&start, &end));
    }
    g_n_moves = 0;
    g_n_misses = 0;
    g_n_hits = 0;

    if (g.inPhase2()) {
      g = onoro::Game<n_pawns>(g, p2_move);
    } else {
      g = onoro::Game<n_pawns>(g, p1_move);
    }
    printf("%s\n", g.PrintDiff(prev).c_str());

    if (g.isFinished()) {
      printf("%s won\n", g.blackWins() ? "black" : "white");
      break;
    }

    prev = g;
  }

  printf("Table size: %zu\n", m.table().size());

  return 0;
}

int main(int argc, char* argv[]) {
  static constexpr const uint32_t N = 8;

  absl::ParseCommandLine(argc, argv);

  printf("score size: %zu\n", sizeof(onoro::Score));
  printf("score align: %zu\n", alignof(onoro::Score));

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
  /*g1.setTile(onoro::idx_t(8, 8), onoro::Game<N>::TileState::TILE_WHITE);
  g1.sum_of_mass_ += (onoro::HexPos){ 12, 8 };
  g1.state_.turn++;
  g1.state_.blackTurn = 1;*/

  onoro::Game<N> g2;
  /*g2.clearTile(onoro::idx_t(7, 7));
  g2.clearTile(onoro::idx_t(7, 8));
  g2.setTile(onoro::idx_t(7, 6), onoro::Game<N>::TileState::TILE_WHITE);
  g2.setTile(onoro::idx_t(7, 7), onoro::Game<N>::TileState::TILE_BLACK);
  g2.setTile(onoro::idx_t(6, 6), onoro::Game<N>::TileState::TILE_WHITE);
  g2.sum_of_mass_ += (onoro::HexPos){ 8, 4 };
  g2.state_.turn++;
  g2.state_.blackTurn = 1;*/

  if (!g1.validate() || !g2.validate()) {
    return -1;
  } else {
    printf("Valid states\n");
  }

  /*
  onoro::GameView<N> v1(&g1);
  onoro::GameView<N> v2(&g2, K4(C2(0), C2(0)), false);

  printf("%s\n", v1.game().Print().c_str());
  printf("%s\n", v1.game().Print2().c_str());
  printf("%s\n", v2.game().Print().c_str());
  printf("%s\n", v2.game().Print2().c_str());
  printf("Hash 1: %s\n", onoro::GameHash<N>::printK4Hash(v1.hash()).c_str());
  printf("Hash 2: %s\n", onoro::GameHash<N>::printK4Hash(v2.hash()).c_str());

  GameEq<N> eq;
  printf("Are equal? %s\n", eq(v1, v2) ? "true" : "false");
  printf("Are equal? %s\n", eq(v2, v1) ? "true" : "false");
  */

  return 0;
}
