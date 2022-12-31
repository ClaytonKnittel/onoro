#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "game_state.pb.h"
#include "hash_group.h"
#include "hex_pos.h"
#include "union_find.h"
#include "utils/fun/print_colors.h"

namespace onoro {

using namespace hash_group;

/*
 * (x, y) coordinates as an index.
 *
 * The game played on a hexagonal grid, but is internally represented as a 2D
 * cartesian grid, indexed as shown:
 *
 *  (0,0)  (1,0)  (2,0)  (3,0) ...
 *
 *     (0,1)  (1,1)  (2,1)  (3,1) ...
 *
 *         (0,2)  (1,2)  (2,2)  (3,2) ...
 *
 *              ...
 *
 * The x and y coordinates can range from 0 to num_pawns - 1.
 */
class idx_t {
 private:
  uint8_t _bytes;

  constexpr explicit idx_t(uint8_t bytes) : _bytes(bytes) {}

 public:
  constexpr idx_t() : _bytes(0) {}
  constexpr idx_t(uint32_t x, uint32_t y) : _bytes(x | (y << 4)) {}

  constexpr idx_t operator+(idx_t other) const {
    // Assume no overflow
    return idx_t(_bytes + other._bytes);
  }

  constexpr idx_t operator+=(idx_t other) {
    *this = (*this + other);
    return *this;
  }

  constexpr bool operator==(idx_t other) const {
    return _bytes == other._bytes;
  }

  constexpr bool operator!=(idx_t other) const {
    return !(*this == other);
  }

  static constexpr idx_t null_idx() {
    return idx_t(0x00u);
  }

  // Constructs an idx_t that will increase/decrease the value of the x
  // coordinate of an idx_t by "i" when added to it.
  static constexpr idx_t add_x(int32_t i) {
    // For negative i, let the bits above 0-3 overflow into the y "slot", so
    // that upon subtraction the y slot will remain unchanged (so long as x is
    // not smaller than abs(i)).
    return idx_t(static_cast<uint8_t>(i));
  }

  // Constructs an idx_t that will increase/decrease the value of the y
  // coordinate of an idx_t by "i" when added to it.
  static constexpr idx_t add_y(int32_t i) {
    return idx_t(static_cast<uint8_t>(i << 4));
  }

  constexpr uint32_t x() const {
    return _bytes & 0x0fu;
  }

  constexpr void set_x(uint32_t x) {
    _bytes = static_cast<uint8_t>((_bytes & 0xf0u) | x);
  }

  constexpr uint32_t y() const {
    return (_bytes & 0xf0u) >> 4;
  }

  constexpr void set_y(uint32_t y) {
    _bytes = static_cast<uint8_t>((_bytes & 0x0fu) | (y << 4));
  }

  constexpr uint8_t get_bytes() const {
    return _bytes;
  }
};

static constexpr const uint32_t MAX_IDX = 15;

class [[gnu::packed]] Score {
 private:
  constexpr Score(bool cur_player_wins, uint32_t turn_count_tie,
                  uint32_t turn_count_win)
      : turn_count_win_(static_cast<uint16_t>(turn_count_win)),
        turn_count_tie_(static_cast<uint16_t>(turn_count_tie)),
        score_(cur_player_wins ? 1 : 0) {}

 public:
  /*
   * Constructs a score with no information.
   */
  constexpr Score() : turn_count_win_(0), turn_count_tie_(0), score_(0) {}

  constexpr Score(const Score&) = default;

  constexpr Score& operator=(const Score&) = default;

  constexpr bool operator==(const Score& score) const {
    return turn_count_win_ == score.turn_count_win_ &&
           turn_count_tie_ == score.turn_count_tie_ && score_ == score.score_;
  }

  static constexpr Score nil() {
    return Score();
  }

  static constexpr Score win(uint32_t turn_count_win) {
    return Score(true, 0, turn_count_win);
  }

  static constexpr Score lose(uint32_t turn_count_lose) {
    return Score(false, 0, turn_count_lose);
  }

  static constexpr Score tie(uint32_t turn_count_tie) {
    return Score(false, turn_count_tie, 0);
  }

  /*
   * Used to mark a game state as an ancestor of the current tree being
   * explored. Will be overwritten with the actual score once its calculation is
   * finished.
   */
  static constexpr Score ancestor() {
    // Mark the current player as winning with turn_count_win_ = 0, which is an
    // impossible state to be in.
    return Score(true, 0, 0);
  }

  /*
   * The score of the game given `depth` moves to play.
   */
  constexpr int32_t score(uint32_t depth) const {
    if (depth <= turn_count_tie()) {
      return 0;
    } else if (depth >= turn_count_win()) {
      return turn_count_win() * 2 - 1;
    } else {
      fprintf(stderr, "Attempted to resolve score at undiscovered depth\n");
      abort();
    }
  }

  constexpr uint32_t turn_count_win() const {
    return static_cast<uint32_t>(turn_count_win_);
  }

  constexpr uint32_t turn_count_tie() const {
    return static_cast<uint32_t>(turn_count_tie_);
  }

  /*
   * Transforms a score at a given state of the game to how that score would
   * appear from the perspective of a game state one step before it.
   *
   * If a winning move for one player has been found in n steps, then it is
   * turned into a winning move for the other player in n + 1 steps.
   */
  constexpr Score& backstep() {
    if (turn_count_win_ > 0) {
      turn_count_win_++;
      score_ = !score_;
    }
    turn_count_tie_++;
    return *this;
  }

  /*
   * Merges the information contained in another score into this one. This
   * assumes that the scores are compatible, i.e. they don't contain conflicting
   * information.
   */
  constexpr Score& merge(const Score& score) {
    turn_count_win_ =
        std::min(static_cast<uint32_t>(turn_count_win_) - 1,
                 static_cast<uint32_t>(score.turn_count_win_) - 1) +
        1;
    turn_count_tie_ = std::max(turn_count_tie_, score.turn_count_tie_);
    score_ = score_ || score.score_;
    return *this;
  }

  /*
  constexpr void set_score(int32_t score) {
    score_ = static_cast<int8_t>(score);
  }

  constexpr void set_turn_count_win(uint32_t turn_count_win) {
    turn_count_win_ = static_cast<uint16_t>(turn_count_win);
  }

  constexpr void set_turn_count_tie(uint32_t turn_count_tie) {
    turn_count_tie_ = static_cast<uint16_t>(turn_count_tie);
  }
  */

  /*
   * True if this score can be used in place of a search that goes
   * `search_depth` moves deep (i.e. this score will equal the score calculated
   * by a full search this deep).
   */
  constexpr bool determined(uint32_t search_depth) const {
    return (turn_count_win_ != 0 && search_depth >= turn_count_win_) ||
           search_depth <= turn_count_tie_;
  }

  std::string Print() const {
    if (turn_count_win_ == 0) {
      return absl::StrFormat("[tie:%u]", turn_count_tie());
    } else {
      return absl::StrFormat("[tie:%u,%s:%u]", turn_count_tie(),
                             score_ ? "cur" : "oth", turn_count_win());
    }
  }

 private:
  /*
   * The minimum number of moves that somebody can force a win within. If 0,
   * then no win has been found from this state yet.
   */
  uint32_t turn_count_win_ : 12;
  /*
   * The maximum number of moves ahead that nobody can force a win within.
   */
  uint32_t turn_count_tie_ : 11;

  /*
   * Who can force a win after turn_count_win_ turns, with 1 being the current
   * player and 0 being the other player. This is 0 if no win has been found
   * yet.
   */
  uint32_t score_ : 1;
};

// Forward declare GameHash
template <uint32_t NPawns>
class GameHash;

// Forward declare GameEq
template <uint32_t NPawns>
class GameEq;

template <uint32_t NPawns, typename Hash>
class Game;

struct P1Move {
  template <uint32_t NPawns, typename Hash, class CallbackFn>
  static constexpr bool forEachMoveFn(const Game<NPawns, Hash>& g,
                                      CallbackFn cb) {
    return g.forEachMove(cb);
  }

  // Position to play pawn at.
  idx_t loc;
};

struct P2Move {
  template <uint32_t NPawns, typename Hash, class CallbackFn>
  static constexpr bool forEachMoveFn(const Game<NPawns, Hash>& g,
                                      CallbackFn cb) {
    return g.forEachMoveP2(cb);
  }

  // Position to move pawn to.
  idx_t to;
  // Position in pawn_poses array to move pawn from.
  uint8_t from_idx;
};

template <uint32_t NPawns, typename Hash = GameHash<NPawns>>
class Game {
  template <uint32_t NPawns_>
  friend class GameHash;
  template <uint32_t NPawns_>
  friend class GameEq;

 public:
  enum class TileState {
    TILE_EMPTY = 0,
    TILE_BLACK = 1,
    TILE_WHITE = 2,
  };

  struct BoardSymmetryState {
    /*
     * The group operation to perform on the board before calculating the hash.
     * This is used to align board states on all symmetry axes which the board
     * isn't possibly symmetric about itself.
     */
    D6 op;

    /*
     * The symmetry class this board state belongs in, which depends on where
     * the center of mass lies. If the location of the center of mass is
     * symmetric to itself under some group operations, then those symmetries
     * must be checked when looking up in the hash table.
     */
    SymmetryClass symm_class;

    /*
     * The offset to apply when calculating the integer-coordinate, symmetry
     * invariant "center of mass"
     */
    HexPos center_offset;
  };

  class pawn_iterator {
    friend class Game<NPawns, Hash>;

   public:
    pawn_iterator& operator++() {
      idx_++;
      return *this;
    }

    bool operator==(const pawn_iterator& other) const {
      return idx_ == other.idx_;
    }

    bool operator!=(const pawn_iterator& other) const {
      return !(*this == other);
    }

    idx_t operator*() const {
      return game_->pawn_poses_[idx_];
    }

    bool isBlack() const {
      return !(idx_ & 1);
    }

    /*
     * Returns the index into pawn_poses_ that the current pawn is at.
     */
    uint32_t pawnIdx() const {
      return idx_;
    }

   protected:
    explicit pawn_iterator(const Game<NPawns, Hash>& game)
        : game_(&game), idx_(0) {}
    pawn_iterator(const Game<NPawns, Hash>& game, uint32_t idx)
        : game_(&game), idx_(idx) {}

    pawn_iterator() {}

    static constexpr pawn_iterator end(const Game<NPawns, Hash>& game) {
      pawn_iterator it;
      it.game_ = nullptr;
      it.idx_ = game.nPawnsInPlay();
      return it;
    }

   protected:
    const Game<NPawns, Hash>* game_;

    // index of the current pawn in the list of pawn_poses_
    uint32_t idx_;
  };

  class color_pawn_iterator : public pawn_iterator {
    friend class Game<NPawns, Hash>;

   public:
    // Constructs the begin() iterator.
    color_pawn_iterator(const Game<NPawns, Hash>& game, bool black)
        : pawn_iterator(game, black ? 0 : 1) {}

    color_pawn_iterator& operator++() {
      this->pawn_iterator::idx_ += 2;
      return *this;
    }

   private:
    // default constructor is for the end() iterator
    color_pawn_iterator() {}

    static constexpr color_pawn_iterator end(const Game<NPawns, Hash>& game,
                                             bool black) {
      uint32_t n_pawns = game.nPawnsInPlay();
      color_pawn_iterator it;
      it.pawn_iterator::game_ = nullptr;

      // Calculate the index the iterator will land on knowing it jumps by two.
      it.pawn_iterator::idx_ =
          n_pawns + ((static_cast<uint32_t>(!black) ^ n_pawns) & 1);
      return it;
    }
  };

 private:
  struct GameState {
    // You can play this game with a max of 8 pawns, and turn count stops
    // incrementing after the end of phase 1
    uint8_t turn       : 4;
    uint8_t blackTurn  : 1;
    uint8_t finished   : 1;
    uint8_t hashed     : 1;
    uint8_t __reserved : 1;
  };

  // bits per entry in the board
  static constexpr uint32_t bits_per_uint64 = 64;
  static constexpr uint32_t bits_per_tile = 2;
  static constexpr uint64_t tile_bitmask = (1 << bits_per_tile) - 1;
  static constexpr uint32_t max_pawns_per_player = 8;
  static constexpr uint32_t min_neighbors_per_pawn = 2;
  static constexpr uint32_t n_in_row_to_win = 4;

  enum class COMOffset {
    // Offset by (0, 0)
    x0y0,
    // Offset by (1, 0)
    x1y0,
    // Offset by (0, 1)
    x0y1,
    // Offset by (1, 1)
    x1y1,
  };

  /*
   * Table of offsets to apply when calculating the integer-coordinate, symmetry
   * invariant "center of mass"
   *
   * Mapping from regions of the tiling unit square to the offset from the
   * coordinate in the bottom left corner of the unit square to the center of
   * the hex tile this region is a part of, indexed by the D6 symmetry op
   * associated with the region. See the description of genSymmStateTable() for
   * this mapping from symmetry op to region..
   */
  static constexpr COMOffset boardSymmStateOpToCOMOffset[D6::order()] = {
    // r0
    COMOffset::x0y0,
    // r1
    COMOffset::x0y1,
    // r2
    COMOffset::x1y1,
    // r3
    COMOffset::x1y1,
    // r4
    COMOffset::x1y0,
    // r5
    COMOffset::x0y0,
    // s0
    COMOffset::x0y1,
    // s1
    COMOffset::x0y0,
    // s2
    COMOffset::x0y0,
    // s3
    COMOffset::x1y0,
    // s4
    COMOffset::x1y1,
    // s5
    COMOffset::x1y1,
  };

  /*
   * Compressed format of BoardSymmetryState to be stored in the board symmetry
   * state table.
   *
   * Layout:
   *  first 4 bits: op.ordinal()
   *  next 3 bits: symm_class
   *  last bit: unused
   *
   * Center offset can be computed using the boardSymmStateOpToCOMOffset table.
   */
  class BoardSymmStateData {
   public:
    constexpr BoardSymmStateData() : data_(0) {}
    constexpr BoardSymmStateData(D6 op, SymmetryClass symm_class)
        : data_(static_cast<uint8_t>(
              op.ordinal() | (static_cast<uint32_t>(symm_class) << 4))) {}

    static constexpr HexPos COMOffsetToHexPos(COMOffset offset) {
      switch (offset) {
        case COMOffset::x0y0: {
          return { 0, 0 };
        }
        case COMOffset::x1y0: {
          return { 1, 0 };
        }
        case COMOffset::x0y1: {
          return { 0, 1 };
        }
        case COMOffset::x1y1: {
          return { 1, 1 };
        }
        default: {
          __builtin_unreachable();
        }
      }
    }

    BoardSymmetryState parseSymmetryState() const {
      uint32_t symm_class = static_cast<uint32_t>(data_) & 0x0fu;
      return (BoardSymmetryState){
        D6(symm_class), static_cast<SymmetryClass>(data_ >> 4),
        COMOffsetToHexPos(boardSymmStateOpToCOMOffset[symm_class])
      };
    }

   private:
    uint8_t data_;
  };

  // Returns the width of the game board. This is also the upper bound on the
  // x/y values in idx_t.
  static constexpr uint32_t getBoardWidth();

  static constexpr uint32_t getBoardSize();

  static constexpr uint32_t getSymmStateTableWidth();

  // Returns the size of the symm state table, in terms of number of elements.
  static constexpr uint32_t getSymmStateTableSize();

  /*
   * Returns the symmetry state operation corresponding to the point (x, y) in
   * the unit square scaled by n_pawns.
   *
   * n_pawns is the number of pawns currently in play.
   *
   * (x, y) are elements of {0, 1, ... n_pawns-1} x {0, 1, ... n_pawns-1}
   */
  static constexpr D6 symmStateOp(uint32_t x, uint32_t y, uint32_t n_pawns);

  /*
   * Returns the symmetry class corresponding to the point (x, y) in the unit
   * square scaled by n_pawns.
   *
   * n_pawns is the number of pawns currently in play.
   *
   * (x, y) are elements of {0, 1, ... n_pawns-1} x {0, 1, ... n_pawns-1}
   */
  static constexpr SymmetryClass symmStateClass(uint32_t x, uint32_t y,
                                                uint32_t n_pawns);

  static constexpr std::pair<idx_t, HexPos> calcMoveShift(idx_t move);

 public:
  Game();
  Game(const Game&) = default;
  Game(Game&&) = default;

  Game& operator=(const Game&) = default;
  Game& operator=(Game&&) = default;

  bool operator==(const Game&) const;

  // Make a move
  // Phase 1: place a pawn
  Game(const Game&, P1Move move);
  // Phase 2: move a pawn from somewhere to somewhere else
  Game(const Game&, P2Move move);

  std::string Print() const;
  std::string PrintDiff(const Game<NPawns, Hash>& other) const;
  std::string Print2() const;
  onoro::proto::GameState SerializeState() const;

  static absl::StatusOr<Game> LoadState(const onoro::proto::GameState& state);

  bool validate() const;

  static void printSymmStateTableOps(uint32_t n_reps = 1);
  static void printSymmStateTableSymms(uint32_t n_reps = 1);

 public:  // TODO revert to private
  static constexpr std::array<BoardSymmStateData, getSymmStateTableSize()>
  genSymmStateTable();

  void appendTile(idx_t pos);

  // Changes the value of a tile at location "i" in the pawn_poses_ array
  void moveTile(idx_t, uint32_t i);

  /*
   * Returns true if the last move made (passed in as last_move) caused a win.
   */
  bool checkWin(idx_t last_move) const;

  // Shifts all pawns of game by the given offset
  constexpr void shiftTiles(idx_t offset);

  /*
   * The board symmetry state data table is a cache of the BoardSymmStateData
   * values for points (x % NPawns, y % NPawns). Since the unit square
   * in HexPos coordinates ((0, 0) to (1, 1)) tiles the plane correctly, we only
   * need the symmetry states for one instance of this tiling, with points
   * looking up their folded mapping onto the representative tile.
   *
   * The reason for NPawns * NPawns entries is that the position of the
   * center of mass modulo the width/height of the unit square determines a
   * board's symmetry state, and the average of NPawns (the number of pawns in
   * play in phase 2 of the game) is a multiple of 1 / NPawns.
   */
  static constexpr std::array<BoardSymmStateData, getSymmStateTableSize()>
      symm_state_table = genSymmStateTable();

  /*
   * Array of indexes of pawn positions. Odd entries (even index) are black
   * pawns, the others are white. Filled from lowest to highest index as the
   * first phase proceeds.
   */
  std::array<idx_t, NPawns> pawn_poses_;

  GameState state_;

  /*
   * Optional: can store the game score here to save space.
   */
  Score score_;

  // Sum of all HexPos's of pieces on the board
  HexPos16 sum_of_mass_;

  std::size_t hash_;

 public:
  // Returns an ordinal for the given index. Ordinals are a unique mapping from
  // idx_t to non-negative integers exactly covering the range [0,
  // num_possible_indexes - 1].
  static constexpr uint32_t idxOrd(idx_t idx);

  static constexpr idx_t ordToIdx(uint32_t ord);

  static constexpr HexPos idxToPos(idx_t idx);

  static constexpr idx_t posToIdx(HexPos pos);

  uint32_t nPawnsInPlay() const;

  bool blackTurn() const;

  bool inPhase2() const;

  std::size_t hash() const;

  TileState getTile(idx_t idx) const;

  // Returns the idx_t for the pawn at position i in pawn_poses_
  idx_t idxAt(uint32_t i) const;

  // returns true if black won, given isFinished() == true
  bool blackWins() const;

  bool isFinished() const;

  template <class CallbackFnT>
  bool forEachNeighbor(idx_t idx, CallbackFnT cb) const;

  // Iterates over only neighbors above/to the left of idx.
  template <class CallbackFnT>
  bool forEachTopLeftNeighbor(idx_t idx, CallbackFnT cb) const;

  template <class CallbackFnT>
  bool forEachMove(CallbackFnT cb) const;

  // calls cb with arguments to : idx_t, from : idx_t
  template <class CallbackFnT>
  bool forEachMoveP2(CallbackFnT cb) const;

  Score getScore() const;

  /*
   * Technically not const, but doesn't modify the game state in any way, it
   * just setting an auxiliary field.
   */
  void setScore(Score score) const;

  BoardSymmetryState calcSymmetryState() const;

  /*
   * Gives the chosen origin tile for the board given the BoardSymmetryState.
   * The origin is guaranteed to be the same tile for equivalent boards under
   * symmetries.
   */
  constexpr HexPos originTile(const BoardSymmetryState& state) const;

  /*
   * Iterators over the pawns.
   */
  pawn_iterator pawns_begin() const;
  constexpr pawn_iterator pawns_end() const;

  color_pawn_iterator color_pawns_begin(bool black) const;
  constexpr color_pawn_iterator color_pawns_end(bool black) const;

  /*
   * Iterates over all pawns on the board, calling cb with the idx_t of the pawn
   * as the only argument. If cb returns false, iteration halts and this method
   * returns false.
   */
  template <class CallbackFnT>
  bool forEachPawn(CallbackFnT cb) const;

  /*
   * Iterates over all pawns on the board belonging to the black (black = true)
   * or white (black = false) player, calling cb with the idx_t of the pawn as
   * the only argument. If cb returns false, iteration halts and this method
   * returns false.
   */
  template <class CallbackFnT>
  bool forEachPlayerPawn(bool black, CallbackFnT cb) const;
};

template <uint32_t NPawns, typename Hash>
constexpr uint32_t Game<NPawns, Hash>::getBoardWidth() {
  return NPawns;
}

template <uint32_t NPawns, typename Hash>
constexpr uint32_t Game<NPawns, Hash>::getBoardSize() {
  return getBoardWidth() * getBoardWidth();
}

template <uint32_t NPawns, typename Hash>
constexpr uint32_t Game<NPawns, Hash>::getSymmStateTableWidth() {
  return NPawns;
}

template <uint32_t NPawns, typename Hash>
constexpr uint32_t Game<NPawns, Hash>::getSymmStateTableSize() {
  return getSymmStateTableWidth() * getSymmStateTableWidth();
}

template <uint32_t NPawns, typename Hash>
constexpr D6 Game<NPawns, Hash>::symmStateOp(uint32_t x, uint32_t y,
                                             uint32_t n_pawns) {
  // (x2, y2) is (x, y) folded across the line y = x
  uint32_t x2 = std::max(x, y);
  uint32_t y2 = std::min(x, y);

  // (x3, y3) is (x2, y2) folded across the line y = n_pawns - x
  uint32_t x3 = std::min(x2, n_pawns - y2);
  uint32_t y3 = std::min(y2, n_pawns - x2);

  bool c1 = y < x;
  bool c2 = x2 + y2 < n_pawns;
  bool c3a = y3 + n_pawns <= 2 * x3;
  bool c3b = 2 * y3 <= x3;

  if (c1) {
    if (c2) {
      if (c3a) {
        return D6(D6::Action::REFL, 3);
      } else if (c3b) {
        return D6(D6::Action::ROT, 0);
      } else {
        return D6(D6::Action::REFL, 1);
      }
    } else {
      if (c3a) {
        return D6(D6::Action::ROT, 4);
      } else if (c3b) {
        return D6(D6::Action::REFL, 5);
      } else {
        return D6(D6::Action::ROT, 2);
      }
    }
  } else {
    if (c2) {
      if (c3a) {
        return D6(D6::Action::ROT, 1);
      } else if (c3b) {
        return D6(D6::Action::REFL, 2);
      } else {
        return D6(D6::Action::ROT, 5);
      }
    } else {
      if (c3a) {
        return D6(D6::Action::REFL, 0);
      } else if (c3b) {
        return D6(D6::Action::ROT, 3);
      } else {
        return D6(D6::Action::REFL, 4);
      }
    }
  }
}

template <uint32_t NPawns, typename Hash>
constexpr SymmetryClass Game<NPawns, Hash>::symmStateClass(uint32_t x,
                                                           uint32_t y,
                                                           uint32_t n_pawns) {
  // (x2, y2) is (x, y) folded across the line y = x
  uint32_t x2 = std::max(x, y);
  uint32_t y2 = std::min(x, y);

  // (x3, y3) is (x2, y2) folded across the line y = n_pawns - x
  uint32_t x3 = std::min(x2, n_pawns - y2);
  uint32_t y3 = std::min(y2, n_pawns - x2);

  // Calculate the symmetry class of this position.
  if (x == 0 && y == 0) {
    return SymmetryClass::C;
  } else if (3 * x2 == 2 * n_pawns && 3 * y2 == n_pawns) {
    return SymmetryClass::V;
  } else if (2 * x2 == n_pawns && (y2 == 0 || 2 * y2 == n_pawns)) {
    return SymmetryClass::E;
  } else if (2 * y3 == x3 || (x2 + y2 == n_pawns && 3 * y2 < n_pawns)) {
    return SymmetryClass::CV;
  } else if (x2 == y2 || y2 == 0) {
    return SymmetryClass::CE;
  } else if (y3 + n_pawns == 2 * x3 ||
             (x2 + y2 == n_pawns && 3 * y2 > n_pawns)) {
    return SymmetryClass::EV;
  } else {
    return SymmetryClass::TRIVIAL;
  }
}

template <uint32_t NPawns, typename Hash>
constexpr std::pair<idx_t, HexPos> Game<NPawns, Hash>::calcMoveShift(
    idx_t move) {
  idx_t offset(0, 0);
  HexPos hex_offset{ 0, 0 };

  if (move.y() == 0) {
    offset = idx_t::add_y(1);
    hex_offset = HexPos{ 0, 1 };
  } else if (move.y() == getBoardWidth() - 1) {
    offset = idx_t::add_y(-1);
    hex_offset = HexPos{ 0, -1 };
  }
  if (move.x() == 0) {
    offset += idx_t::add_x(1);
    hex_offset += HexPos{ 1, 0 };
  } else if (move.x() == getBoardWidth() - 1) {
    offset += idx_t::add_x(-1);
    hex_offset = HexPos{ -1, 0 };
  }

  return { offset, hex_offset };
}

// Black goes first, but since black has 2 forced moves and white only has 1,
// white is effectively first to make a choice.
template <uint32_t NPawns, typename Hash>
Game<NPawns, Hash>::Game()
    : state_({ 0xfu, 1, 0, 0, 0 }), sum_of_mass_{ 0, 0 } {
  static_assert(NPawns <= 2 * max_pawns_per_player);

  for (uint32_t i = 0; i < NPawns; i++) {
    pawn_poses_[i] = idx_t::null_idx();
  }

  int32_t mid_idx = (getBoardWidth() - 1) / 2;

  if (true) {
    idx_t b_start(mid_idx, mid_idx);
    idx_t w_start(mid_idx + 1, mid_idx + 1);
    idx_t b_next(mid_idx + 1, mid_idx);

    appendTile(b_start);
    appendTile(w_start);
    appendTile(b_next);
  } else {
    idx_t b_start(mid_idx + 1, mid_idx - 1);
    idx_t w_start(mid_idx + 1, mid_idx);
    idx_t b_next(mid_idx + 1, mid_idx + 1);
    idx_t w_next(mid_idx, mid_idx + 1);
    idx_t b_next2(mid_idx - 1, mid_idx);
    idx_t w_next2(mid_idx, mid_idx - 1);

    appendTile(b_start);
    appendTile(w_start);
    appendTile(b_next);
    appendTile(w_next);
    appendTile(b_next2);
    appendTile(w_next2);
  }
}

template <uint32_t NPawns, typename Hash>
Game<NPawns, Hash>::Game(const Game<NPawns, Hash>& g, P1Move move)
    : pawn_poses_(g.pawn_poses_),
      state_(g.state_),
      sum_of_mass_(g.sum_of_mass_) {
  appendTile(move.loc);

  auto [offset, hex_offset] = calcMoveShift(move.loc);
  shiftTiles(offset);
  sum_of_mass_ += static_cast<HexPos16>(nPawnsInPlay() * hex_offset);

  state_.finished = checkWin(move.loc + offset);
}

template <uint32_t NPawns, typename Hash>
Game<NPawns, Hash>::Game(const Game& g, P2Move move)
    : pawn_poses_(g.pawn_poses_),
      state_(g.state_),
      sum_of_mass_(g.sum_of_mass_) {
  moveTile(move.to, move.from_idx);

  auto [offset, hex_offset] = calcMoveShift(move.to);
  shiftTiles(offset);
  sum_of_mass_ += static_cast<HexPos16>(NPawns * hex_offset);

  state_.finished = checkWin(move.to + offset);
}

template <uint32_t NPawns, typename Hash>
std::string Game<NPawns, Hash>::Print() const {
  static const char tile_str[3] = {
    '.',
    'B',
    'W',
  };

  std::ostringstream ostr;
  for (uint32_t y = getBoardWidth() - 1; y < getBoardWidth(); y--) {
    ostr << std::setw(getBoardWidth() - 1 - y) << "";

    for (uint32_t x = 0; x < getBoardWidth(); x++) {
      ostr << tile_str[static_cast<int>(getTile(idx_t(x, y)))];
      if (x < getBoardWidth() - 1) {
        ostr << " ";
      }
    }

    if (y > 0) {
      ostr << "\n";
    }
  }
  return ostr.str();
}

template <uint32_t NPawns, typename Hash>
std::string Game<NPawns, Hash>::PrintDiff(
    const Game<NPawns, Hash>& other) const {
  static const char tile_str[3] = {
    '.',
    'B',
    'W',
  };

  HexPos bl_corner = { NPawns, NPawns };
  HexPos bl_corner_other = { NPawns, NPawns };

  forEachPawn([&bl_corner](idx_t idx) {
    HexPos pos = idxToPos(idx);

    bl_corner = { std::min(pos.x, bl_corner.x), std::min(pos.y, bl_corner.y) };
    return true;
  });

  other.forEachPawn([&bl_corner_other](idx_t idx) {
    HexPos pos = idxToPos(idx);

    bl_corner_other = { std::min(pos.x, bl_corner_other.x),
                        std::min(pos.y, bl_corner_other.y) };
    return true;
  });

  // Technically there are some cases where this isn't enough to disambiguate
  // where a move was made to, but those are rare so we can ignore them.
  for (HexPos off : (HexPos[]){
           { 0, 0 },   { 1, 0 },   { 1, 1 },  { 0, 1 },  { -1, 0 },
           { -1, -1 }, { 0, -1 },  { 2, 0 },  { 2, 1 },  { 2, 2 },
           { 1, 2 },   { 0, 2 },   { -1, 1 }, { -2, 0 }, { -2, -1 },
           { -2, -2 }, { -1, -2 }, { 0, -2 }, { 1, -1 },
       }) {
    uint32_t n_missing = 0;
    uint32_t n_new = 0;
    std::ostringstream ostr;
    for (uint32_t y = getBoardWidth() - 1; y < getBoardWidth(); y--) {
      ostr << std::setw(getBoardWidth() - 1 - y) << "";

      for (uint32_t x = 0; x < getBoardWidth(); x++) {
        idx_t trans =
            posToIdx(idxToPos(idx_t(x, y)) - bl_corner + bl_corner_other + off);
        TileState other_tile = other.getTile(trans);
        TileState tile = getTile(idx_t(x, y));

        if (other_tile != TileState::TILE_EMPTY &&
            tile == TileState::TILE_EMPTY) {
          n_missing++;
          ostr << P_RED;
        } else if (other_tile == TileState::TILE_EMPTY &&
                   tile != TileState::TILE_EMPTY) {
          n_new++;
          ostr << P_GREEN;
        }

        ostr << tile_str[static_cast<int>(tile)] << P_DEFAULT;
        if (x < getBoardWidth() - 1) {
          ostr << " ";
        }
      }

      if (y > 0) {
        ostr << "\n";
      }
    }

    if (n_missing > 1 || n_new != 1) {
      continue;
    }
    return ostr.str();
  }
  return "ERROR: no way to get between those two game states in one move!";
}

template <uint32_t NPawns, typename Hash>
std::string Game<NPawns, Hash>::Print2() const {
  static const char* tile_str[3] = {
    P_256_BG_COLOR(7),
    P_256_BG_COLOR(4),
    P_256_BG_COLOR(1),
  };

  HexPos origin = originTile(calcSymmetryState());

  std::ostringstream ostr;
  for (uint32_t y = getBoardWidth() - 1; y < getBoardWidth(); y--) {
    ostr << std::setw(2) << y << std::setw(getBoardWidth() - 2 * (y / 2) - 1)
         << "";

    for (uint32_t x = 0; x < getBoardWidth(); x++) {
      ostr << tile_str[static_cast<uint32_t>(getTile(idx_t(x, y)))];

      HexPos tile = idxToPos(idx_t(x, y));
      if (tile == origin) {
        ostr << "x";
      } else if (y == 0) {
        ostr << x % 10;
      } else {
        ostr << "_";
      }
      ostr << P_256_BG_DEFAULT;

      if (x < getBoardWidth() - 1) {
        ostr << " ";
      }
    }

    if (y > 0) {
      ostr << "\n";
    }
  }
  return ostr.str();
}

template <uint32_t NPawns, typename Hash>
onoro::proto::GameState Game<NPawns, Hash>::SerializeState() const {
  onoro::proto::GameState state;

  state.set_black_turn(blackTurn());
  state.set_turn_num(state_.turn);
  state.set_finished(state_.finished);

  HexPos bl_corner{ INT32_MAX, INT32_MAX };
  for (auto it = pawns_begin(); it != pawns_end(); ++it) {
    HexPos pos = idxToPos(*it);
    bl_corner =
        HexPos{ std::min(bl_corner.x, pos.x), std::min(bl_corner.y, pos.y) };
  }

  for (auto it = pawns_begin(); it != pawns_end(); ++it) {
    HexPos rel_pos = idxToPos(*it) - bl_corner;

    onoro::proto::GameState::Pawn& pawn = *state.add_pawns();
    pawn.set_x(rel_pos.x);
    pawn.set_y(rel_pos.y);
    pawn.set_black(it.isBlack());
  }

  return state;
}

template <uint32_t NPawns, typename Hash>
absl::StatusOr<Game<NPawns, Hash>> Game<NPawns, Hash>::LoadState(
    const onoro::proto::GameState& state) {
  Game g;
  g.state_ = (Game::GameState){
    .turn = 0xf,
    .blackTurn = 1,
    .finished = 0,
    .hashed = 0,
  };
  g.sum_of_mass_ = (HexPos16){ 0, 0 };

  if (state.turn_num() < NPawns - 1 &&
      static_cast<int>(state.turn_num()) != state.pawns_size() - 1) {
    return absl::InternalError(absl::StrFormat(
        "Unexpected num pawns for turn %d (found %d, expect %d)",
        state.turn_num(), state.pawns_size(), state.turn_num() + 1));
  }

  std::vector<onoro::proto::GameState::Pawn> black_pawns;
  std::vector<onoro::proto::GameState::Pawn> white_pawns;

  for (const auto& pawn : state.pawns()) {
    if (pawn.black()) {
      black_pawns.push_back(pawn);
    } else {
      white_pawns.push_back(pawn);
    }
  }

  if (static_cast<uint64_t>(black_pawns.size() - white_pawns.size()) > 1) {
    return absl::InternalError(
        absl::StrFormat("Unexpected number of black/white pawns, have %zu and "
                        "%zu, but expect %zu and %zu",
                        black_pawns.size(), white_pawns.size(),
                        (state.pawns_size() + 1) / 2, state.pawns_size() / 2));
  }

  HexPos minPos{ INT32_MAX, INT32_MAX };
  HexPos maxPos{ INT32_MIN, INT32_MIN };

  for (const onoro::proto::GameState::Pawn& pawn : state.pawns()) {
    minPos =
        HexPos{ std::min(minPos.x, pawn.x()), std::min(minPos.y, pawn.y()) };
    maxPos =
        HexPos{ std::min(maxPos.x, pawn.x()), std::min(maxPos.y, pawn.y()) };
  }

  HexPos mid = (minPos + maxPos) / 2u;
  HexPos shift = HexPos{ NPawns / 2 - 1, NPawns / 2 - 1 } - mid;

  for (uint32_t i = 0; i < static_cast<uint32_t>(state.pawns_size()); i++) {
    onoro::proto::GameState::Pawn pawn;
    if (i % 2 == 0) {
      pawn = black_pawns[i / 2];
    } else {
      pawn = white_pawns[i / 2];
    }

    HexPos p{ static_cast<int32_t>(pawn.x()), static_cast<int32_t>(pawn.y()) };
    idx_t idx = posToIdx(p + shift);
    g.appendTile(idx);

    auto [off, hex_off] = calcMoveShift(idx);
    g.shiftTiles(off);
    g.sum_of_mass_ += static_cast<HexPos16>(g.nPawnsInPlay() * hex_off);

    if (i == static_cast<uint32_t>(state.pawns_size()) - 1) {
      g.state_.finished = g.checkWin(idx + off);
    }

    shift += hex_off;
  }

  if (g.state_.turn != state.turn_num()) {
    return absl::InternalError(
        absl::StrFormat("Pawns imply turn %u, but have turn %u in state",
                        g.state_.turn, state.turn_num()));
  }
  if (state.turn_num() < NPawns - 1) {
    if (g.state_.blackTurn != state.black_turn()) {
      return absl::InternalError(
          absl::StrFormat("Expected %s turn, but state has %s turn",
                          g.state_.blackTurn ? "black" : "white",
                          state.black_turn() ? "black" : "white"));
    }
  } else {
    g.state_.blackTurn = state.black_turn();
  }
  // We have to trust this field in the proto since the last placed piece may
  // not have been the winning move.
  g.state_.finished = state.finished();

  return g;
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::validate() const {
  uint32_t n_b_pawns = 0;
  uint32_t n_w_pawns = 0;
  HexPos sum_of_mass = { 0, 0 };

  UnionFind<uint32_t> uf(getBoardSize());

  if (!forEachPawn(
          [this, &n_b_pawns, &n_w_pawns, &sum_of_mass, &uf](idx_t idx) {
            sum_of_mass += idxToPos(idx);

            if (getTile(idx) == TileState::TILE_BLACK) {
              n_b_pawns++;
            } else if (getTile(idx) == TileState::TILE_WHITE) {
              n_w_pawns++;
            } else {
              printf("Unexpected empty tile at (%d, %d)\n", idx.x(), idx.y());
              return false;
            }

            // Union this pawn with its neigbors
            forEachTopLeftNeighbor(idx, [&uf, idx, this](idx_t neighbor_idx) {
              if (getTile(neighbor_idx) != TileState::TILE_EMPTY) {
                uf.Union(idxOrd(idx), idxOrd(neighbor_idx));
              }
              return true;
            });

            return true;
          })) {
    return false;
  }

  if (n_b_pawns + n_w_pawns != nPawnsInPlay()) {
    printf("Expected %u pawns in play, but found %u\n", nPawnsInPlay(),
           n_b_pawns + n_w_pawns);
    return false;
  }

  if (state_.turn < NPawns - 1 && state_.blackTurn != (state_.turn & 1)) {
    printf("Expected black turn to be %u, but was %u\n", (state_.turn & 1),
           state_.blackTurn);
    return false;
  }

  if (n_b_pawns != n_w_pawns + !state_.blackTurn) {
    printf("Expected %u black pawns and %u white pawns, but found %u and %u\n",
           (nPawnsInPlay() + 1) / 2, nPawnsInPlay() / 2, n_b_pawns, n_w_pawns);
    return false;
  }

  if (sum_of_mass != static_cast<HexPos>(sum_of_mass_)) {
    printf("Sum of mass not correct: expect (%d, %d), but have (%d, %d)\n",
           sum_of_mass.x, sum_of_mass.y, sum_of_mass_.x, sum_of_mass_.y);
    return false;
  }

  uint32_t n_empty_tiles = getBoardSize() - nPawnsInPlay();
  uint32_t n_pawn_groups = uf.GetNumGroups() - n_empty_tiles;

  if (n_pawn_groups != 1) {
    printf("Expected 1 contiguous pawn group, but found %u\n", n_pawn_groups);
    return false;
  }

  return true;
}

template <uint32_t NPawns, typename Hash>
void Game<NPawns, Hash>::printSymmStateTableOps(uint32_t n_reps) {
  static constexpr const uint32_t id[D6::order()] = {
    1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14,
  };

  constexpr uint32_t N = getSymmStateTableWidth();

  for (uint32_t y = n_reps * N - 1; y < n_reps * N; y--) {
    for (uint32_t x = 0; x < n_reps * N; x++) {
      BoardSymmStateData d = symm_state_table[(x % N) + (y % N) * N];
      BoardSymmetryState s = d.parseSymmetryState();

      // clang-format off
      printf(P_256_BG_COLOR(%u) "  " P_256_BG_DEFAULT, id[s.op.ordinal()]);
      // clang-format on
    }
    printf("\n");
  }
}

template <uint32_t NPawns, typename Hash>
void Game<NPawns, Hash>::printSymmStateTableSymms(uint32_t n_reps) {
  static constexpr const uint32_t id[7] = {
    1, 2, 3, 4, 5, 6, 7,
  };

  constexpr uint32_t N = getSymmStateTableWidth();

  for (uint32_t y = n_reps * N - 1; y < n_reps * N; y--) {
    for (uint32_t x = 0; x < n_reps * N; x++) {
      BoardSymmStateData d = symm_state_table[(x % N) + (y % N) * N];
      BoardSymmetryState s = d.parseSymmetryState();

      // clang-format off
      printf(P_256_BG_COLOR(%u) "  " P_256_BG_DEFAULT,
             id[static_cast<uint32_t>(s.symm_class)]);
      // clang-format on
    }
    printf("\n");
  }
}

/*
 * The purpose of the symmetry table is to provide a quick way to canonicalize
 * boards when computing and checking for symmetries. Since the center of mass
 * transforms the same as tiles under symmetry operations, we can use the
 * position of the center of mass to prune the list of possible layouts of
 * boards symmetric to this one. For example, if the center of mass does not
 * lie on any symmetry lines, then if we orient the center of mass in the same
 * segment of the origin hexagon, all other game boards which are symmetric to
 * this one will have oriented their center of masses to the same position,
 * meaning the coordinates of all pawns in both boards will be the same.
 *
 * We choose to place the center of mass within the triangle extending from
 * the center of the origin hex to the center of its right edge (+x), and up
 * to its top-right vertex. This triangle has coordinates (0, 0), (1/2, 0),
 * (2/3, 1/3) in HexPos space.
 *
 * A unit square centered at (1/2, 1/2) in HexPos space is a possible unit
 * tile for the hexagonal grid (keep in mind that the hexagons are not regular
 * hexagons in HexPos space). Pictured below is a mapping from regions on this
 * unit square to D6 operations (about the origin) to transform the points
 * within the corresponding region to a point within the designated triangle
 * defined above.
 *
 * +-------------------------------+
 * |`            /    r3     _ _ | |
 * |  `    s0   /       _ _    |   |
 * |    `      /   _ _       |     |
 * |  r1  `   / _          |       |
 * |     _ _`v     s4    |        /|
 * |  _     / `        |         / |
 * e       /    `    |     r2   /  |
 * |  s2  /       `e           /   |
 * |     /  r5   |  `         / s5 |
 * |    /      |      `      /    -|
 * |   /     |    s1    `   /- -   |
 * |  /    |            - `v    r4 |
 * | /   |         - -    / `      |
 * |/  |      - -        /    `    |
 * | |   - -      r0    /  s3   `  |
 * +-------------------e-----------+
 *
 * This image is composed of lines:
 *  y = 2x
 *  y = 1/2(x + 1)
 *  y = x
 *  y = 1 - x
 *  y = 1/2x
 *  y = 2x - 1
 *
 * These lines divie the unit square into 12 equally-sized regions in
 * cartesian space, and listed in each region is the D6 group operation to map
 * that region to the designated triangle.
 *
 * Since the lines given above are the symmetry lines of the hexagonal grid,
 * we can use them to determine which symmetry group the board state belongs
 * in.
 *
 * Let (x, y) = (n_pawns * (com.x % 1), n_pawns * (com.y % 1)) be the folded
 * center of mass within the unit square, scaled by n_pawns in play. Note that
 * x and y are integers.
 *
 * Let (x2, y2) = (max(x, y), min(x, y)) be (x, y) folded across the symmetry
 * line y = x. Note that the diagram above is also symmetryc about y = x, save
 * for the group operations in the regions.
 *
 * - C is the symmetry group D6 about the origin, which is only possible when
 *     the center of mass lies on the origin, so (x, y) = (0, 0).
 * - V is the symmetry group D3 about a vertex, which are labeled as 'v' in
 *     the diagram. These are the points (2/3 n_pawns, 1/3 n_pawns) and (1/3
 *     n_pawns, 2/3 n_pawns), or (x2, y2) = (2/3 n_pawns, 1/3 n_pawns).
 * - E is the symmetry group K4 about the center of an edge, which are labeled
 *     as 'e' in the diagram. These are the points (1/2 n_pawns, 0), (1/2
 *     n_pawns, 1/2 n_pawns), and (0, 1/2 n_pawns), or (x2, y2) =
 *     (1/2 n_pawns, 0) or (1/2 n_pawns, 1/2 n_pawns).
 * - CV is the symmetry group C2 about a line passing through the center of
 *     the origin hex and one of its vertices.
 * - CE is the symmetry group C2 about a line passing through the center of
 *     the origin hex and the center of one of its edges.
 * - EV is the symmetry group C2 about a line tangent to one of the edges of
 *     the origin hex.
 * - TRIVIAL is a group with no symmetries other than the identity, so all
 *     board states with center of masses which don't lie on any symmetry
 *     lines are part of this group.
 *
 * In the case that the center of mass lies on a symmetry line/point, it is
 * classified into one of 6 symmetry groups above. These symmetry groups are
 * subgroups of D6, and are uniquely defined by the remaining symmetries after
 * canonicalizing the symmetry line/point by the operations given in the
 * graphic. As an example, the e's on the graphic will all be mapped to the e in
 * the bottom center of the graphic, but there are 4 possible orientations of
 * the board with this constraint applied. The group of these 4 orientations is
 * K4 (C2 + C2), which is precisely the symmetries of the infinite hexagonal
 * grid centered at the midpoint of an edge (nix translation). This also means
 * that it does not matter which of the 4 group operations we choose to apply to
 * the game state when canonicalizing if the center of mass lies on an e, since
 * they are symmetries of each other in this K4 group.
 */
template <uint32_t NPawns, typename Hash>
constexpr std::array<typename Game<NPawns, Hash>::BoardSymmStateData,
                     Game<NPawns, Hash>::getSymmStateTableSize()>
Game<NPawns, Hash>::genSymmStateTable() {
  constexpr uint32_t N = getSymmStateTableWidth();

  std::array<BoardSymmStateData, getSymmStateTableSize()> table;

  for (uint32_t x = 0; x < N; x++) {
    for (uint32_t y = 0; y < N; y++) {
      table[x + y * N] =
          BoardSymmStateData(symmStateOp(x, y, N), symmStateClass(x, y, N));
    }
  }

  return table;
}

template <uint32_t NPawns, typename Hash>
void Game<NPawns, Hash>::appendTile(idx_t pos) {
  state_.turn++;
  pawn_poses_[state_.turn] = pos;

  state_.blackTurn = !state_.blackTurn;
  state_.hashed = 0;
  sum_of_mass_ += static_cast<HexPos16>(idxToPos(pos));
}

template <uint32_t NPawns, typename Hash>
void Game<NPawns, Hash>::moveTile(idx_t pos, uint32_t i) {
  idx_t old_idx = pawn_poses_[i];
  pawn_poses_[i] = pos;

  state_.blackTurn = !state_.blackTurn;
  state_.hashed = 0;
  sum_of_mass_ += static_cast<HexPos16>(idxToPos(pos) - idxToPos(old_idx));
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::checkWin(idx_t last_move) const {
  // Check for a win in all 3 directions
  HexPos last_move_pos = idxToPos(last_move);

  // Bitvector of positions occupied by pawns of this color along the 3 lines
  // extending out from last_move. Intentionally leave a zero bit between each
  // of the 3 sets so they can't form a continuous string of 1's across borders.
  // - s[0-15]: line running along the x-axis, with bit i corresponding to
  //     (x, i)
  // - s[17-32]: line running along the line x = y, with bit i corresponding to
  //     (x - min(x, y) + i, y - min(x, y) + i).
  // - s[34-49]: line running along the y-axis, with bit i corresponding to
  //     (i, y)
  uint64_t s =
      (UINT64_C(0x1) << last_move_pos.x) |
      (UINT64_C(0x20000) << std::min(last_move_pos.x, last_move_pos.y)) |
      (UINT64_C(0x400000000) << last_move_pos.y);

  // Unsafe pawn iteration: rely on the fact that idx_t::null_idx() will not
  // complete a line in the first phase of the game (can't reach the border
  // without being able to move pawns), and for phase two, all pawns are placed,
  // so this is safe.
  for (uint32_t i = blackTurn() ? 1 : 0; i < NPawns; i += 2) {
    idx_t idx = pawn_poses_[i];
    HexPos pos = idxToPos(idx);
    HexPos delta = pos - last_move_pos;
    if (delta.y == 0) {
      s = s | (UINT64_C(0x1) << pos.x);
    } else if (delta.x == delta.y) {
      s = s | (UINT64_C(0x20000) << std::min(pos.x, pos.y));
    } else if (delta.x == 0) {
      s = s | (UINT64_C(0x400000000) << pos.y);
    }
  }

  // Check if any 4 bits in a row are set:
  s = (s & (s << 2));
  s = (s & (s << 1));

  return s != 0;
}

template <uint32_t NPawns, typename Hash>
constexpr void Game<NPawns, Hash>::shiftTiles(idx_t offset) {
  if (offset != idx_t(0, 0)) {
    for (uint32_t i = 0; i < nPawnsInPlay(); i++) {
      if (pawn_poses_[i] != idx_t::null_idx()) {
        pawn_poses_[i] += offset;
      }
    }
  }
  state_.hashed = 0;
}

template <uint32_t NPawns, typename Hash>
constexpr uint32_t Game<NPawns, Hash>::idxOrd(idx_t idx) {
  return idx.x() + idx.y() * NPawns;
}

template <uint32_t NPawns, typename Hash>
constexpr idx_t Game<NPawns, Hash>::ordToIdx(uint32_t ord) {
  return idx_t(ord % NPawns, ord / NPawns);
}

template <uint32_t NPawns, typename Hash>
constexpr HexPos Game<NPawns, Hash>::idxToPos(idx_t idx) {
  return { static_cast<int32_t>(idx.x()), static_cast<int32_t>(idx.y()) };
}

template <uint32_t NPawns, typename Hash>
constexpr idx_t Game<NPawns, Hash>::posToIdx(HexPos pos) {
  return idx_t(static_cast<uint32_t>(pos.x), static_cast<uint32_t>(pos.y));
}

template <uint32_t NPawns, typename Hash>
uint32_t Game<NPawns, Hash>::nPawnsInPlay() const {
  return state_.turn + 1;
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::blackTurn() const {
  return state_.blackTurn;
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::inPhase2() const {
  return state_.turn == NPawns - 1;
}

template <uint32_t NPawns, typename Hash>
std::size_t Game<NPawns, Hash>::hash() const {
  if (!state_.hashed) {
    const_cast<Game<NPawns>*>(this)->hash_ = Hash::calcHash(*this);
    const_cast<Game<NPawns>*>(this)->state_.hashed = 1;
  }
  return hash_;
}

template <uint32_t NPawns, typename Hash>
typename Game<NPawns, Hash>::TileState Game<NPawns, Hash>::getTile(
    idx_t idx) const {
  uint64_t i;
  const uint64_t* pawn_poses =
      reinterpret_cast<const uint64_t*>(pawn_poses_.data());

  if (idx == idx_t::null_idx()) {
    return TileState::TILE_EMPTY;
  }

  uint64_t mask = static_cast<uint64_t>(idx.get_bytes());
  mask = mask | (mask << 8);
  mask = mask | (mask << 16);
  mask = mask | (mask << 32);

  for (i = 0; i < NPawns / 8; i++) {
    uint64_t xor_search = mask ^ pawn_poses[i];

    uint64_t zero_mask = (xor_search - UINT64_C(0x0101010101010101)) &
                         ~xor_search & UINT64_C(0x8080808080808080);
    if (zero_mask != 0) {
      uint32_t set_bit_idx = __builtin_ctzl(zero_mask);
      // Black has the even indices, white has the odd.
      return ((set_bit_idx / 8) & 0x1) ? TileState::TILE_WHITE
                                       : TileState::TILE_BLACK;
    }
  }

  // only necessary if NPawns not a multiple of eight
  for (i = 8 * i; i < NPawns; i++) {
    if (this->pawn_poses_[i] == idx) {
      return (i & 0x1) ? TileState::TILE_WHITE : TileState::TILE_BLACK;
    }
  }

  return TileState::TILE_EMPTY;
}

template <uint32_t NPawns, typename Hash>
idx_t Game<NPawns, Hash>::idxAt(uint32_t i) const {
  return pawn_poses_[i];
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::blackWins() const {
  return !state_.blackTurn;
}

template <uint32_t NPawns, typename Hash>
bool Game<NPawns, Hash>::isFinished() const {
  return state_.finished;
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachNeighbor(idx_t idx, CallbackFnT cb) const {
  // clang-format off
#define CB_OR_RET(...)    \
    if (!cb(__VA_ARGS__)) \
      return false;
  // clang-format on

  CB_OR_RET(idx + idx_t::add_x(-1) + idx_t::add_y(-1));
  CB_OR_RET(idx + idx_t::add_y(-1));
  CB_OR_RET(idx + idx_t::add_x(-1));
  CB_OR_RET(idx + idx_t::add_x(1));
  CB_OR_RET(idx + idx_t::add_y(1));
  CB_OR_RET(idx + idx_t::add_x(1) + idx_t::add_y(1));

  return true;
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachTopLeftNeighbor(idx_t idx,
                                                CallbackFnT cb) const {
  // clang-format off
#define CB_OR_RET(...)    \
    if (!cb(__VA_ARGS__)) \
      return false;
  // clang-format on

  CB_OR_RET(idx + idx_t::add_x(-1) + idx_t::add_y(-1));
  CB_OR_RET(idx + idx_t::add_y(-1));
  CB_OR_RET(idx + idx_t::add_x(-1));

  return true;
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachMove(CallbackFnT cb) const {
  assert(!inPhase2());
  static constexpr uint32_t tmp_board_tile_bits = 2;
  static constexpr uint64_t tmp_board_tile_mask =
      (uint64_t(1) << tmp_board_tile_bits) - 1;

  constexpr uint64_t tmp_board_len =
      (getBoardSize() * tmp_board_tile_bits + bits_per_uint64 - 1) /
      bits_per_uint64;
  // bitvector of moves already taken
  uint64_t tmp_board[tmp_board_len] = { 0 };

  return forEachPawn([this, &tmp_board, &cb](idx_t next_idx) {
    return forEachNeighbor(next_idx, [&tmp_board, this,
                                      &cb](idx_t neighbor_idx) {
      if (getTile(neighbor_idx) == TileState::TILE_EMPTY) {
        uint32_t ord = idxOrd(neighbor_idx);
        uint32_t tb_shift = tmp_board_tile_bits *
                            (ord % (bits_per_uint64 / tmp_board_tile_bits));
        uint64_t tbb = tmp_board[ord / (bits_per_uint64 / tmp_board_tile_bits)];
        uint64_t mask = tmp_board_tile_mask << tb_shift;
        uint64_t full_mask = uint64_t(min_neighbors_per_pawn) << tb_shift;

        if ((tbb & mask) != full_mask) {
          tbb += (uint64_t(1) << tb_shift);
          tmp_board[ord / (bits_per_uint64 / tmp_board_tile_bits)] = tbb;

          if ((tbb & mask) == full_mask) {
            if (!cb((P1Move){ neighbor_idx })) {
              return false;
            }
          }
        }
      }

      return true;
    });
  });
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachMoveP2(CallbackFnT cb) const {
  assert(inPhase2());
  static constexpr uint32_t tmp_board_tile_bits = 2;
  static constexpr uint64_t tmp_board_tile_mask =
      (uint64_t(1) << tmp_board_tile_bits) - 1;

  constexpr uint64_t tmp_board_len =
      (getBoardSize() * tmp_board_tile_bits + bits_per_uint64 - 1) /
      bits_per_uint64;
  // bitvector of moves already taken
  uint64_t tmp_board[tmp_board_len] = { 0 };

  // One pass to populate tmp_board with neighbor counts
  forEachPawn([this, &tmp_board](idx_t next_idx) {
    forEachNeighbor(next_idx, [&tmp_board](idx_t neighbor_idx) {
      uint32_t idx = idxOrd(neighbor_idx);
      uint32_t tb_shift =
          tmp_board_tile_bits * (idx % (bits_per_uint64 / tmp_board_tile_bits));
      uint64_t tbb = tmp_board[idx / (bits_per_uint64 / tmp_board_tile_bits)];
      uint64_t mask = tmp_board_tile_mask << tb_shift;
      uint64_t full_mask = uint64_t(min_neighbors_per_pawn + 1) << tb_shift;

      if ((tbb & mask) != full_mask) {
        tbb += (uint64_t(1) << tb_shift);
        tmp_board[idx / (bits_per_uint64 / tmp_board_tile_bits)] = tbb;
      }

      return true;
    });

    return true;
  });

  // Another pass to enumerate all moves
  for (color_pawn_iterator it = color_pawns_begin(blackTurn());
       it != color_pawns_end(blackTurn()); ++it) {
    UnionFind<uint32_t> uf(getBoardSize());

    idx_t next_idx = *it;

    // Calculate the number of disjoint pawn groups after removing the pawn at
    // next_idx
    forEachPawn([&uf, &next_idx, this](idx_t idx) {
      uint32_t idx_val = idxOrd(idx);

      // Skip ourself
      if (idx == next_idx) {
        return true;
      }

      forEachTopLeftNeighbor(
          idx, [&uf, &next_idx, &idx_val, this](idx_t neighbor_idx) {
            if (getTile(neighbor_idx) != TileState::TILE_EMPTY &&
                neighbor_idx != next_idx) {
              uf.Union(idx_val, idxOrd(neighbor_idx));
            }
            return true;
          });

      return true;
    });

    uint32_t n_empty_tiles = getBoardSize() - nPawnsInPlay();
    // the pawn we are moving is its own group
    uint32_t n_pawn_groups = uf.GetNumGroups() - n_empty_tiles - 1;

    // number of neighbors with 1 neighbor after removing this piece
    uint32_t n_to_satisfy = 0;
    // decrease neighbor count of all neighbors
    forEachNeighbor(next_idx, [&tmp_board, &n_to_satisfy,
                               this](idx_t neighbor_idx) {
      uint32_t neighbor_ord = idxOrd(neighbor_idx);
      uint32_t tb_idx = neighbor_ord / (bits_per_uint64 / tmp_board_tile_bits);
      uint32_t tb_shift =
          tmp_board_tile_bits *
          (neighbor_ord % (bits_per_uint64 / tmp_board_tile_bits));

      tmp_board[tb_idx] -= uint64_t(1) << tb_shift;
      if (((tmp_board[tb_idx] >> tb_shift) & tmp_board_tile_mask) == 1 &&
          getTile(neighbor_idx) != TileState::TILE_EMPTY) {
        n_to_satisfy++;
      }

      return true;
    });

    // try all possible new locations for piece
    for (uint64_t j = 0; j < tmp_board_len; j++) {
      uint64_t tmp_board_bitmask = tmp_board[j];
      uint64_t tmp_bitmask_idx_off =
          j * (bits_per_uint64 / tmp_board_tile_bits);

      while (tmp_board_bitmask != 0) {
        uint32_t next_idx_ord_off =
            __builtin_ctzl(tmp_board_bitmask) / tmp_board_tile_bits;
        uint32_t tb_shift = next_idx_ord_off * tmp_board_tile_bits;
        uint32_t next_idx_ord = next_idx_ord_off + tmp_bitmask_idx_off;
        uint64_t clr_mask = tmp_board_tile_mask << tb_shift;

        // skip this tile if it isn't empty (this will also skip the piece's
        // old location since we haven't removed it, which we want)
        if (getTile(ordToIdx(next_idx_ord)) != TileState::TILE_EMPTY ||
            ((tmp_board_bitmask >> tb_shift) & tmp_board_tile_mask) <= 1) {
          tmp_board_bitmask = tmp_board_bitmask & ~clr_mask;
          continue;
        }

        tmp_board_bitmask = tmp_board_bitmask & ~clr_mask;

        uint32_t n_satisfied = 0;
        uint32_t g1 = -1u;
        uint32_t g2 = -1u;
        uint32_t groups_touching = 0;
        forEachNeighbor(ordToIdx(next_idx_ord), [&tmp_board, &next_idx,
                                                 &n_satisfied, &uf, &g1, &g2,
                                                 &groups_touching,
                                                 this](idx_t neighbor_idx) {
          uint32_t neighbor_ord = idxOrd(neighbor_idx);
          if (getTile(neighbor_idx) == TileState::TILE_EMPTY) {
            return true;
          }

          uint32_t tb_idx =
              neighbor_ord / (bits_per_uint64 / tmp_board_tile_bits);
          uint32_t tb_shift =
              tmp_board_tile_bits *
              (neighbor_ord % (bits_per_uint64 / tmp_board_tile_bits));

          if (((tmp_board[tb_idx] >> tb_shift) & tmp_board_tile_mask) == 1) {
            n_satisfied++;
          }

          if (neighbor_idx != next_idx) {
            uint32_t group_id = uf.GetRoot(neighbor_ord);
            if (group_id != g1) {
              if (g1 == -1u) {
                g1 = group_id;
                groups_touching++;
              } else if (group_id != g2) {
                g2 = group_id;
                groups_touching++;
              }
            }
          }
          return true;
        });

        if (n_satisfied == n_to_satisfy && groups_touching == n_pawn_groups) {
          if (!cb((P2Move){ ordToIdx(next_idx_ord),
                            static_cast<uint8_t>(it.pawnIdx()) })) {
            return false;
          }
        }
      }
    }

    // increase neighbor count of all neighbors
    forEachNeighbor(next_idx, [&tmp_board](idx_t neighbor_idx) {
      uint32_t neighbor_ord = idxOrd(neighbor_idx);
      uint32_t tb_idx = neighbor_ord / (bits_per_uint64 / tmp_board_tile_bits);
      uint32_t tb_shift =
          tmp_board_tile_bits *
          (neighbor_ord % (bits_per_uint64 / tmp_board_tile_bits));

      tmp_board[tb_idx] += (uint64_t(1) << tb_shift);
      return true;
    });
  }

  return true;
}

template <uint32_t NPawns, typename Hash>
Score Game<NPawns, Hash>::getScore() const {
  return score_;
}

template <uint32_t NPawns, typename Hash>
void Game<NPawns, Hash>::setScore(Score score) const {
  const_cast<Game*>(this)->score_ = score;
}

template <uint32_t NPawns, typename Hash>
typename Game<NPawns, Hash>::BoardSymmetryState
Game<NPawns, Hash>::calcSymmetryState() const {
  auto [x, y] = sum_of_mass_;
  uint32_t n_pawns = nPawnsInPlay();

  if (n_pawns == NPawns) {
    x %= NPawns;
    y %= NPawns;

    return symm_state_table[x + y * getSymmStateTableWidth()]
        .parseSymmetryState();
  } else {
    x %= n_pawns;
    y %= n_pawns;

    D6 op = symmStateOp(x, y, n_pawns);
    SymmetryClass symm_class = symmStateClass(x, y, n_pawns);
    HexPos center_off = BoardSymmStateData::COMOffsetToHexPos(
        boardSymmStateOpToCOMOffset[op.ordinal()]);

    return { op, symm_class, center_off };
  }
}

template <uint32_t NPawns, typename Hash>
constexpr HexPos Game<NPawns, Hash>::originTile(
    const typename Game<NPawns, Hash>::BoardSymmetryState& state) const {
  // Operate under the assumption that x, y >= 0
  uint32_t x = static_cast<uint32_t>(sum_of_mass_.x);
  uint32_t y = static_cast<uint32_t>(sum_of_mass_.y);
  uint32_t n_pawns = nPawnsInPlay();
  HexPos truncated_com = { static_cast<int32_t>(x / n_pawns),
                           static_cast<int32_t>(y / n_pawns) };
  return truncated_com + state.center_offset;
}

template <uint32_t NPawns, typename Hash>
typename Game<NPawns, Hash>::pawn_iterator Game<NPawns, Hash>::pawns_begin()
    const {
  typename Game<NPawns, Hash>::pawn_iterator it = pawn_iterator(*this);
  return it;
}

template <uint32_t NPawns, typename Hash>
constexpr typename Game<NPawns, Hash>::pawn_iterator
Game<NPawns, Hash>::pawns_end() const {
  return Game<NPawns, Hash>::pawn_iterator::end(*this);
}

template <uint32_t NPawns, typename Hash>
typename Game<NPawns, Hash>::color_pawn_iterator
Game<NPawns, Hash>::color_pawns_begin(bool black) const {
  typename Game<NPawns, Hash>::color_pawn_iterator it =
      color_pawn_iterator(*this, black);
  return it;
}

template <uint32_t NPawns, typename Hash>
constexpr typename Game<NPawns, Hash>::color_pawn_iterator
Game<NPawns, Hash>::color_pawns_end(bool black) const {
  return Game<NPawns, Hash>::color_pawn_iterator::end(*this, black);
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachPawn(CallbackFnT cb) const {
  for (pawn_iterator it = pawns_begin(); it != pawns_end(); ++it) {
    if (!cb(*it)) {
      return false;
    }
  }

  return true;
}

template <uint32_t NPawns, typename Hash>
template <class CallbackFnT>
bool Game<NPawns, Hash>::forEachPlayerPawn(bool black, CallbackFnT cb) const {
  const color_pawn_iterator end = color_pawns_end(black);
  for (color_pawn_iterator it = color_pawns_begin(black); it != end; ++it) {
    if (!cb(*it)) {
      return false;
    }
  }

  return true;
}
}  // namespace onoro
