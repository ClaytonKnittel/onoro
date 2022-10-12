#pragma once

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include "hex_pos.h"
#include "union_find.h"
#include "utils/fun/print_colors.h"

namespace onoro {

// (x, y) coordinates as an index.
typedef std::pair<int32_t, int32_t> idx_t;

// Forward declare GameHash
template <uint32_t NPawns>
class GameHash;

template <uint32_t NPawns>
class Game;

template <uint32_t NPawns>
class GameEq {
 public:
  GameEq() = default;

  bool operator()(const Game<NPawns>& g1,
                  const Game<NPawns>& g2) const noexcept;
};

template <uint32_t NPawns>
class Game {
  friend class GameHash<NPawns>;
  friend class GameEq<NPawns>;

 private:
  enum class TileState {
    TILE_EMPTY = 0,
    TILE_BLACK = 1,
    TILE_WHITE = 2,
  };

  struct GameState {
    // You can play this game with a max of 8 pawns, and turn count stops
    // incrementing after the end of phase 1
    uint8_t turn       : 4;
    uint8_t blackTurn  : 1;
    uint8_t finished   : 1;
    uint8_t __reserved : 2;
  };

  // bits per entry in the board
  static constexpr uint32_t bits_per_entry = 64;
  static constexpr uint32_t bits_per_tile = 2;
  static constexpr uint64_t tile_bitmask = (1 << bits_per_tile) - 1;
  static constexpr uint32_t max_pawns_per_player = 8;
  static constexpr uint32_t min_neighbors_per_pawn = 2;
  static constexpr uint32_t n_in_row_to_win = 4;

  enum class SymmetryClass {
    // Center of mass lies in the center of a hexagonal tile.
    C,
    // Center of mass lies on a vertex of a hexagonal tile.
    V,
    // Center of mass lies on the midpoint of an edge of a hexagonal tile.
    E,
    // Center of mass lies on a line connecting the center of a hexagonal tile
    // to one of its vertices.
    CV,
    // Center of mass lies on a line connecting the center of a hexagonal tile
    // to the midpoint of one if its edges.
    CE,
    // Center of mass lies on the edge of a hexagonal tile.
    EV,
    // Center of mass is none of the above.
    TRIVIAL,
  };

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
    COMOffset::x0y1,
    // r1
    COMOffset::x1y1,
    // r2
    COMOffset::x1y1,
    // r3
    COMOffset::x1y0,
    // r4
    COMOffset::x0y0,
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

  /*
   * Calculates the required size of board for a game with n_pawns pawns.
   */
  static constexpr uint32_t getBoardLen();

  /*
   * Calculates the number of tiles in the board.
   */
  static constexpr uint32_t getBoardNumTiles();

  /*
   * Calculates the size of the board in terms of uint64_t's.
   */
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

  // copies src into dst, shifting the bits in
  // src left by "bit_offset" (with overflow propagated)
  // bit_offset may be negative and any size (so long as its absolute value is <
  // 64 * n_8bytes)
  static void copyAndShift(uint64_t* __restrict__ dst,
                           const uint64_t* __restrict__ src, size_t n_8bytes,
                           int32_t bit_offset);

  static std::pair<int, HexPos> calcMoveShiftAndOffset(const Game&, idx_t move);

 public:
  Game();
  Game(const Game&) = default;
  Game(Game&&) = default;

  Game& operator=(Game&&) = default;

  // Make a move
  // Phase 1: place a pawn
  Game(const Game&, idx_t move);
  // Phase 2: move a pawn from somewhere to somewhere else
  Game(const Game&, idx_t move, idx_t from);

  std::string Print() const;

  static void printSymmStateTableOps(uint32_t n_reps = 1);
  static void printSymmStateTableSymms(uint32_t n_reps = 1);

 private:
  static constexpr std::array<BoardSymmStateData, getSymmStateTableSize()>
  genSymmStateTable();

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

  uint64_t board_[getBoardSize()];

  GameState state_;

  // Sum of all HexPos's of pieces on the board
  HexPos sum_of_mass_;

 public:
  /*
   * Converts an absolute index to an idx_t.
   */
  static idx_t toIdx(uint32_t i);

  /*
   * Converts an idx_t to an absolute index.
   */
  static uint32_t fromIdx(idx_t idx);

  static HexPos idxToPos(idx_t idx);

  static idx_t posToIdx(HexPos pos);

  static bool inBounds(idx_t idx);

  uint32_t nPawnsInPlay() const;

  bool inPhase2() const;

  TileState getTile(uint32_t i) const;
  TileState getTile(idx_t idx) const;

  void setTile(uint32_t i, TileState);
  void setTile(idx_t idx, TileState);

  void clearTile(idx_t idx);

  bool isFinished() const;

  // returns true if black won, given isFinished() == true
  bool blackWins() const;

  /*
   * Returns true if the last move made (passed in as last_move) caused a win.
   */
  bool checkWin(idx_t last_move) const;

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

 private:
  /*
   * Returns the truncated center of mass of the grid (i.e. the center of mass,
   * rounded down to the nearest integer coordinates).
   */
  HexPos truncatedCenter() const;

  /*
   * Iterates over all pawns on the board, calling cb with the idx_t of the pawn
   * as the only argument. If cb returns false, iteration halts and this method
   * returns false.
   */
  template <class CallbackFnT>
  bool forEachPawn(CallbackFnT cb) const;

  /*
   * Iterates over all pawns on the board belonging to the player whose turn it
   * currrently is, calling cb with the idx_t of the pawn as the only argument.
   * If cb returns false, iteration halts and this method returns false.
   */
  template <class CallbackFnT>
  bool forEachPlayablePawn(CallbackFnT cb) const;
};

template <uint32_t NPawns>
bool GameEq<NPawns>::operator()(const Game<NPawns>& g1,
                                const Game<NPawns>& g2) const noexcept {
  return true;
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getBoardLen() {
  return NPawns - 1;
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getBoardNumTiles() {
  return getBoardLen() * getBoardLen();
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getBoardSize() {
  return (getBoardNumTiles() * bits_per_tile + bits_per_entry - 1) /
         bits_per_entry;
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getSymmStateTableWidth() {
  return NPawns;
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getSymmStateTableSize() {
  return getSymmStateTableWidth() * getSymmStateTableWidth();
}

template <uint32_t NPawns>
constexpr D6 Game<NPawns>::symmStateOp(uint32_t x, uint32_t y,
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
        return D6(D6::Action::ROT, 3);
      } else if (c3b) {
        return D6(D6::Action::REFL, 1);
      } else {
        return D6(D6::Action::ROT, 5);
      }
    } else {
      if (c3a) {
        return D6(D6::Action::REFL, 3);
      } else if (c3b) {
        return D6(D6::Action::ROT, 1);
      } else {
        return D6(D6::Action::REFL, 5);
      }
    }
  } else {
    if (c2) {
      if (c3a) {
        return D6(D6::Action::REFL, 0);
      } else if (c3b) {
        return D6(D6::Action::ROT, 4);
      } else {
        return D6(D6::Action::REFL, 2);
      }
    } else {
      if (c3a) {
        return D6(D6::Action::ROT, 0);
      } else if (c3b) {
        return D6(D6::Action::REFL, 4);
      } else {
        return D6(D6::Action::ROT, 2);
      }
    }
  }
}

template <uint32_t NPawns>
constexpr typename Game<NPawns>::SymmetryClass Game<NPawns>::symmStateClass(
    uint32_t x, uint32_t y, uint32_t n_pawns) {
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

template <uint32_t NPawns>
void Game<NPawns>::copyAndShift(uint64_t* __restrict__ dst,
                                const uint64_t* __restrict__ src,
                                size_t n_8bytes, int32_t bit_offset) {
  int32_t offset = bit_offset >> 6;
  uint32_t shift = bit_offset & 0x3f;
  uint32_t rshift = shift == 0 ? 0 : 64 - shift;

  uint64_t r;

  if (shift == 0) {
    if (offset >= 0) {
      memset(dst, 0, offset * sizeof(uint64_t));
      memcpy(dst + offset, src, (n_8bytes - offset) * sizeof(uint64_t));
    } else {
      memcpy(dst, src - offset, (n_8bytes + offset) * sizeof(uint64_t));
      memset(dst + (n_8bytes + offset), 0, -offset * sizeof(uint64_t));
    }
  } else if (offset >= 0) {
    memset(dst, 0, offset * sizeof(uint64_t));

    r = 0;
    for (size_t i = 0; i < n_8bytes - offset; i++) {
      uint64_t b = src[i];
      dst[i + offset] = r | (b << shift);
      r = b >> rshift;
    }
  } else {
    r = src[-(offset + 1)] >> rshift;
    for (size_t i = 0; i < n_8bytes + offset; i++) {
      uint64_t b = src[i - offset];
      dst[i] = r | (b << shift);
      r = b >> rshift;
    }
    dst[n_8bytes + offset] = r;
    memset(dst + (n_8bytes + offset + 1), 0, -(offset + 1) * sizeof(uint64_t));
  }
}

template <uint32_t NPawns>
std::pair<int, HexPos> Game<NPawns>::calcMoveShiftAndOffset(const Game& g,
                                                            idx_t move) {
  int shift = 0;
  HexPos offset = { 0, 0 };
  if (move.second < 0) {
    shift = getBoardLen() * 2;
    offset.x = 1;
    offset.y = 2;
  } else if (move.second > static_cast<int32_t>(getBoardLen() - 1)) {
    shift = -static_cast<int>(getBoardLen() * 2);
    offset.x = -1;
    offset.y = -2;
  }
  if (move.first < 0) {
    shift++;
    offset.x++;
  } else if (move.first > static_cast<int32_t>(getBoardLen() - 1)) {
    shift--;
    offset.x--;
  }

  return { shift, offset };
}

/*
 * The game played on a hexagonal grid, but is internally represented as a 2D
 * cartesian grid, indexed as shown:
 *
 *     0     1     2     3     4   ...
 *
 *  n    n+1   n+2   n+3   n+4     ...
 *
 *     2n  2n+1   2n+2  2n+3  2n+4 ...
 *
 *  3n  3n+1   3n+2  3n+3  3n+4    ...
 */

// Black goes first, but since black has 2 forced moves and white only has 1,
// white is effectively first to make a choice.
template <uint32_t NPawns>
Game<NPawns>::Game() : state_({ 2, 0, 0, 0 }) {
  static_assert(NPawns <= 2 * max_pawns_per_player);

  memset(this->board_, 0, getBoardSize() * sizeof(uint64_t));

  int32_t mid_idx = (getBoardLen() - 1) / 2;

  if (true) {
    idx_t b_start = { mid_idx, mid_idx };
    idx_t w_start = { mid_idx + !(mid_idx & 1), mid_idx + 1 };
    idx_t b_next = { mid_idx + 1, mid_idx };

    setTile(b_start, TileState::TILE_BLACK);
    setTile(w_start, TileState::TILE_WHITE);
    setTile(b_next, TileState::TILE_BLACK);

    sum_of_mass_ = idxToPos(b_start) + idxToPos(w_start) + idxToPos(b_next);
  } else {
    idx_t b_start = { mid_idx + !(mid_idx & 1), mid_idx - 1 };
    idx_t w_start = { mid_idx + !(mid_idx & 1), mid_idx + 1 };
    idx_t b_next = { mid_idx + 1, mid_idx };
    idx_t w_next = { mid_idx - (mid_idx & 1), mid_idx + 1 };
    idx_t b_next2 = { mid_idx - 1, mid_idx };
    idx_t w_next2 = { mid_idx - (mid_idx & 1), mid_idx - 1 };

    setTile(b_start, TileState::TILE_BLACK);
    setTile(w_start, TileState::TILE_BLACK);
    setTile(b_next, TileState::TILE_WHITE);
    setTile(w_next, TileState::TILE_WHITE);
    setTile(b_next2, TileState::TILE_BLACK);
    setTile(w_next2, TileState::TILE_WHITE);

    sum_of_mass_ = idxToPos(b_start) + idxToPos(w_start) + idxToPos(b_next) +
                   idxToPos(w_next) + idxToPos(b_next2) + idxToPos(w_next2);
    state_.turn = 5;
  }
}

template <uint32_t NPawns>
Game<NPawns>::Game(const Game<NPawns>& g, idx_t move)
    : state_({ static_cast<uint8_t>(g.state_.turn + 1), !g.state_.blackTurn, 0,
               0 }) {
  auto [shift, offset] = calcMoveShiftAndOffset(g, move);
  copyAndShift(reinterpret_cast<uint64_t*>(board_),
               reinterpret_cast<const uint64_t*>(g.board_), getBoardSize(),
               shift * bits_per_tile);

  sum_of_mass_ = g.sum_of_mass_ + idxToPos(move) + nPawnsInPlay() * offset;

  setTile(posToIdx(idxToPos(move) + offset),
          g.state_.blackTurn ? TileState::TILE_BLACK : TileState::TILE_WHITE);

  state_.finished = checkWin(move);
}

template <uint32_t NPawns>
Game<NPawns>::Game(const Game& g, idx_t move, idx_t from)
    : state_({ g.state_.turn, !g.state_.blackTurn, 0, 0 }) {
  auto [shift, offset] = calcMoveShiftAndOffset(g, move);
  copyAndShift(reinterpret_cast<uint64_t*>(board_),
               reinterpret_cast<const uint64_t*>(g.board_), getBoardSize(),
               shift * bits_per_tile);

  sum_of_mass_ =
      g.sum_of_mass_ + idxToPos(move) - idxToPos(from) + NPawns * offset;

  setTile(posToIdx(idxToPos(move) + offset),
          g.state_.blackTurn ? TileState::TILE_BLACK : TileState::TILE_WHITE);
  clearTile(posToIdx(idxToPos(from) + offset));

  state_.finished = checkWin(move);
}

template <uint32_t NPawns>
std::string Game<NPawns>::Print() const {
  static const char tile_str[3] = {
    '.',
    'B',
    'W',
  };

  std::ostringstream ostr;
  for (uint32_t y = 0; y < getBoardLen(); y++) {
    if (y % 2 == 0) {
      ostr << " ";
    }

    for (uint32_t x = 0; x < getBoardLen(); x++) {
      ostr << tile_str[static_cast<int>(getTile({ x, y }))];
      if (x < getBoardLen() - 1) {
        ostr << " ";
      }
    }

    if (y < getBoardLen() - 1) {
      ostr << "\n";
    }
  }
  return ostr.str();
}

template <uint32_t NPawns>
void Game<NPawns>::printSymmStateTableOps(uint32_t n_reps) {
  static constexpr const uint32_t id[D6::order()] = {
    1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13,
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

template <uint32_t NPawns>
void Game<NPawns>::printSymmStateTableSymms(uint32_t n_reps) {
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
 * |`            /    s4     _ _ | |
 * |  `    r0   /       _ _    |   |
 * |    `      /   _ _       |     |
 * |  s0  `   / _          |       |
 * |     _ _`v     r2    |        /|
 * |  _     / `        |         / |
 * e       /    `    |     s5   /  |
 * |  r4  /       `e           /   |
 * |     /  s2   |  `         / r1 |
 * |    /      |      `      /    -|
 * |   /     |    r5    `   /- -   |
 * |  /    |            - `v    s3 |
 * | /   |         - -    / `      |
 * |/  |      - -        /    `    |
 * | |   - -      s1    /  r3   `  |
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
template <uint32_t NPawns>
constexpr std::array<typename Game<NPawns>::BoardSymmStateData,
                     Game<NPawns>::getSymmStateTableSize()>
Game<NPawns>::genSymmStateTable() {
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

template <uint32_t NPawns>
idx_t Game<NPawns>::toIdx(uint32_t i) {
  return { i % getBoardLen(), i / getBoardLen() };
}

template <uint32_t NPawns>
uint32_t Game<NPawns>::fromIdx(idx_t idx) {
  return static_cast<uint32_t>(idx.first + idx.second * getBoardLen());
}

template <uint32_t NPawns>
HexPos Game<NPawns>::idxToPos(idx_t idx) {
  return { idx.first + (idx.second >> 1), idx.second };
}

template <uint32_t NPawns>
idx_t Game<NPawns>::posToIdx(HexPos pos) {
  return { pos.x - (pos.y >> 1), pos.y };
}

template <uint32_t NPawns>
bool Game<NPawns>::inBounds(idx_t idx) {
  auto [x, y] = idx;
  return x >= 0 && x < static_cast<int32_t>(getBoardLen()) && y >= 0 &&
         y < static_cast<int32_t>(getBoardLen());
}

template <uint32_t NPawns>
uint32_t Game<NPawns>::nPawnsInPlay() const {
  return state_.turn + 1;
}

template <uint32_t NPawns>
bool Game<NPawns>::inPhase2() const {
  return state_.turn == NPawns - 1;
}

template <uint32_t NPawns>
typename Game<NPawns>::TileState Game<NPawns>::getTile(uint32_t i) const {
  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);

  return TileState((tile_bitv >> (bits_per_tile * bitv_idx)) & tile_bitmask);
}

template <uint32_t NPawns>
typename Game<NPawns>::TileState Game<NPawns>::getTile(idx_t idx) const {
  auto [x, y] = idx;
  uint32_t i = static_cast<uint32_t>(x + y * getBoardLen());

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);

  return TileState((tile_bitv >> (bits_per_tile * bitv_idx)) & tile_bitmask);
}

template <uint32_t NPawns>
void Game<NPawns>::setTile(uint32_t i, TileState piece) {
  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);
  uint64_t bitv_mask =
      (static_cast<uint64_t>(piece) << (bits_per_tile * bitv_idx));

  board_[i * bits_per_tile / bits_per_entry] = tile_bitv | bitv_mask;
}

template <uint32_t NPawns>
void Game<NPawns>::setTile(idx_t idx, TileState piece) {
  uint32_t i = fromIdx(idx);

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);
  uint64_t bitv_mask =
      (static_cast<uint64_t>(piece) << (bits_per_tile * bitv_idx));

  board_[i * bits_per_tile / bits_per_entry] = tile_bitv | bitv_mask;
}

template <uint32_t NPawns>
void Game<NPawns>::clearTile(idx_t idx) {
  uint32_t i = fromIdx(idx);

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);
  uint64_t bitv_mask = ~(tile_bitmask << (bits_per_tile * bitv_idx));

  board_[i * bits_per_tile / bits_per_entry] = tile_bitv & bitv_mask;
}

template <uint32_t NPawns>
bool Game<NPawns>::isFinished() const {
  return state_.finished;
}

template <uint32_t NPawns>
bool Game<NPawns>::blackWins() const {
  return !state_.blackTurn;
}

template <uint32_t NPawns>
bool Game<NPawns>::checkWin(idx_t last_move) const {
  // Check for a win in all 3 directions
  HexPos last_move_pos = idxToPos(last_move);
  uint32_t n_in_row;

  TileState move_color = getTile(last_move);

  {
    n_in_row = 0;
    HexPos last_pos = last_move_pos + (HexPos){ n_in_row_to_win + 1, 0 };
    for (HexPos i = last_move_pos - (HexPos){ n_in_row_to_win, 0 };
         i != last_pos; i += (HexPos){ 1, 0 }) {
      idx_t idx = posToIdx(i);
      if (!inBounds(idx)) {
        continue;
      }

      if (getTile(idx) == move_color) {
        n_in_row++;
      } else {
        n_in_row = 0;
      }

      if (n_in_row == n_in_row_to_win) {
        return true;
      }
    }
  }

  {
    n_in_row = 0;
    HexPos last_pos =
        last_move_pos + (HexPos){ n_in_row_to_win + 1, n_in_row_to_win + 1 };
    for (HexPos i =
             last_move_pos - (HexPos){ n_in_row_to_win, n_in_row_to_win };
         i != last_pos; i += (HexPos){ 1, 1 }) {
      idx_t idx = posToIdx(i);
      if (!inBounds(idx)) {
        continue;
      }

      if (getTile(idx) == move_color) {
        n_in_row++;
      } else {
        n_in_row = 0;
      }

      if (n_in_row == n_in_row_to_win) {
        return true;
      }
    }
  }

  {
    n_in_row = 0;
    HexPos last_pos = last_move_pos + (HexPos){ 0, n_in_row_to_win + 1 };
    for (HexPos i = last_move_pos - (HexPos){ 0, n_in_row_to_win };
         i != last_pos; i += (HexPos){ 0, 1 }) {
      idx_t idx = posToIdx(i);
      if (!inBounds(idx)) {
        continue;
      }

      if (getTile(idx) == move_color) {
        n_in_row++;
      } else {
        n_in_row = 0;
      }

      if (n_in_row == n_in_row_to_win) {
        return true;
      }
    }
  }

  return false;
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachNeighbor(idx_t idx, CallbackFnT cb) const {
  auto [x, y] = idx;

// clang-format off
#define CB_OR_RET(...)    \
    if (!cb(__VA_ARGS__)) \
      return false;
  // clang-format on

  if (y > 0) {
    CB_OR_RET({ x, y - 1 });

    if ((y & 1) == 0) {
      if (x < static_cast<int32_t>(getBoardLen() - 1)) {
        CB_OR_RET({ x + 1, y - 1 });
      }
    } else {
      if (x > 0) {
        CB_OR_RET({ x - 1, y - 1 });
      }
    }
  }
  if (x > 0) {
    CB_OR_RET({ x - 1, y });
  }
  if (x < static_cast<int32_t>(getBoardLen() - 1)) {
    CB_OR_RET({ x + 1, y });
  }
  if (y < static_cast<int32_t>(getBoardLen() - 1)) {
    CB_OR_RET({ x, y + 1 });

    if ((y & 1) == 0) {
      if (x < static_cast<int32_t>(getBoardLen() - 1)) {
        CB_OR_RET({ x + 1, y + 1 });
      }
    } else {
      if (x > 0) {
        CB_OR_RET({ x - 1, y + 1 });
      }
    }
  }

  return true;
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachTopLeftNeighbor(idx_t idx, CallbackFnT cb) const {
  auto [x, y] = idx;

// clang-format off
#define CB_OR_RET(...)    \
    if (!cb(__VA_ARGS__)) \
      return false;
  // clang-format on

  if (y > 0) {
    CB_OR_RET({ x, y - 1 });

    if ((y & 1) == 0) {
      if (x < static_cast<int32_t>(getBoardLen() - 1)) {
        CB_OR_RET({ x + 1, y - 1 });
      }
    } else {
      if (x > 0) {
        CB_OR_RET({ x - 1, y - 1 });
      }
    }
  }
  if (x > 0) {
    CB_OR_RET({ x - 1, y });
  }

  return true;
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachMove(CallbackFnT cb) const {
  static constexpr uint32_t tmp_board_tile_bits = 2;
  static constexpr uint64_t tmp_board_tile_mask =
      (uint64_t(1) << tmp_board_tile_bits) - 1;

  constexpr uint64_t tmp_board_len =
      (getBoardLen() * getBoardLen() * tmp_board_tile_bits + bits_per_entry -
       1) /
      bits_per_entry;
  // bitvector of moves already taken
  uint64_t tmp_board[tmp_board_len];
  memset(tmp_board, 0, tmp_board_len * 8);

  return forEachPawn([this, &tmp_board, &cb](idx_t next_idx) {
    bool res =
        forEachNeighbor(next_idx, [&tmp_board, this, &cb](idx_t neighbor_idx) {
          if (getTile(neighbor_idx) == TileState::TILE_EMPTY) {
            uint32_t idx = fromIdx(neighbor_idx);
            uint32_t tb_shift = tmp_board_tile_bits *
                                (idx % (bits_per_entry / tmp_board_tile_bits));
            uint64_t tbb =
                tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)];
            uint64_t mask = tmp_board_tile_mask << tb_shift;
            uint64_t full_mask = uint64_t(min_neighbors_per_pawn) << tb_shift;

            if ((tbb & mask) != full_mask) {
              tbb += (uint64_t(1) << tb_shift);
              tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)] = tbb;

              if ((tbb & mask) == full_mask) {
                if (!cb(neighbor_idx)) {
                  return false;
                }
              }
            }
          }

          return true;
        });

    return res;
  });
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachMoveP2(CallbackFnT cb) const {
  static constexpr uint32_t tmp_board_tile_bits = 2;
  static constexpr uint64_t tmp_board_tile_mask =
      (uint64_t(1) << tmp_board_tile_bits) - 1;

  constexpr uint64_t tmp_board_len =
      (getBoardLen() * getBoardLen() * tmp_board_tile_bits + bits_per_entry -
       1) /
      bits_per_entry;
  // bitvector of moves already taken
  uint64_t tmp_board[tmp_board_len];
  memset(tmp_board, 0, tmp_board_len * 8);

  // One pass to populate tmp_board with neighbor counts
  forEachPawn([this, &tmp_board](idx_t next_idx) {
    forEachNeighbor(next_idx, [&tmp_board](idx_t neighbor_idx) {
      uint32_t idx = fromIdx(neighbor_idx);
      uint32_t tb_shift =
          tmp_board_tile_bits * (idx % (bits_per_entry / tmp_board_tile_bits));
      uint64_t tbb = tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)];
      uint64_t mask = tmp_board_tile_mask << tb_shift;
      uint64_t full_mask = uint64_t(min_neighbors_per_pawn + 1) << tb_shift;

      if ((tbb & mask) != full_mask) {
        tbb += (uint64_t(1) << tb_shift);
        tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)] = tbb;
      }

      return true;
    });

    return true;
  });

  // Another pass to enumerate all moves
  return forEachPlayablePawn([this, &tmp_board, &cb](idx_t next_idx) {
    UnionFind<uint32_t> uf(getBoardNumTiles());

    // Calculate the number of disjoint pawn groups after removing the pawn at
    // next_idx
    forEachPawn([&uf, &next_idx, this](idx_t idx) {
      uint32_t idx_val = fromIdx(idx);

      // Skip ourself
      if (idx == next_idx) {
        return true;
      }

      forEachTopLeftNeighbor(
          idx, [&uf, &next_idx, &idx_val, this](idx_t neighbor_idx) {
            if (getTile(neighbor_idx) != TileState::TILE_EMPTY &&
                neighbor_idx != next_idx) {
              uf.Union(idx_val, fromIdx(neighbor_idx));
            }
            return true;
          });

      return true;
    });

    uint32_t n_empty_tiles = getBoardNumTiles() - nPawnsInPlay();
    // the pawn we are moving is its own group
    uint32_t n_pawn_groups = uf.GetNumGroups() - n_empty_tiles - 1;

    // number of neighbors with 1 neighbor after removing this piece
    uint32_t n_to_satisfy = 0;
    // decrease neighbor count of all neighbors
    forEachNeighbor(
        next_idx, [&tmp_board, &n_to_satisfy, this](idx_t neighbor_idx) {
          uint32_t idx = fromIdx(neighbor_idx);
          uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
          uint32_t tb_shift = tmp_board_tile_bits *
                              (idx % (bits_per_entry / tmp_board_tile_bits));

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
      uint64_t tmp_bitmask_idx_off = j * (bits_per_entry / tmp_board_tile_bits);

      while (tmp_board_bitmask != 0) {
        uint32_t tmp_next_idx_off =
            __builtin_ctzl(tmp_board_bitmask) / tmp_board_tile_bits;
        uint32_t tb_shift = tmp_next_idx_off * tmp_board_tile_bits;
        uint32_t tmp_next_idx = tmp_next_idx_off + tmp_bitmask_idx_off;
        uint64_t clr_mask2 = tmp_board_tile_mask << tb_shift;

        // skip this tile if it isn't empty (this will also skip the piece's
        // old location since we haven't removed it, which we want)
        if (getTile(tmp_next_idx) != TileState::TILE_EMPTY ||
            ((tmp_board_bitmask >> tb_shift) & tmp_board_tile_mask) <= 1) {
          tmp_board_bitmask = tmp_board_bitmask & ~clr_mask2;
          continue;
        }

        tmp_board_bitmask = tmp_board_bitmask & ~clr_mask2;

        uint32_t n_satisfied = 0;
        uint32_t g1 = -1u;
        uint32_t g2 = -1u;
        uint32_t groups_touching = 0;
        forEachNeighbor(toIdx(tmp_next_idx), [&tmp_board, &next_idx,
                                              &n_satisfied, &uf, &g1, &g2,
                                              &groups_touching,
                                              this](idx_t neighbor_idx) {
          uint32_t idx = fromIdx(neighbor_idx);
          if (getTile(idx) == TileState::TILE_EMPTY) {
            return true;
          }

          uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
          uint32_t tb_shift = tmp_board_tile_bits *
                              (idx % (bits_per_entry / tmp_board_tile_bits));

          if (((tmp_board[tb_idx] >> tb_shift) & tmp_board_tile_mask) == 1) {
            n_satisfied++;
          }

          if (neighbor_idx != next_idx) {
            uint32_t group_id = uf.GetRoot(idx);
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
          if (!cb(toIdx(tmp_next_idx), next_idx)) {
            return false;
          }
        }
      }
    }

    // increase neighbor count of all neighbors
    forEachNeighbor(next_idx, [&tmp_board](idx_t neighbor_idx) {
      uint32_t idx = fromIdx(neighbor_idx);
      uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
      uint32_t tb_shift =
          tmp_board_tile_bits * (idx % (bits_per_entry / tmp_board_tile_bits));

      tmp_board[tb_idx] += (uint64_t(1) << tb_shift);
      return true;
    });

    return true;
  });
}

template <uint32_t NPawns>
HexPos Game<NPawns>::truncatedCenter() const {
  auto [x, y] = sum_of_mass_;
  return HexPos(x / nPawnsInPlay(), y / nPawnsInPlay());
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachPawn(CallbackFnT cb) const {
  for (uint64_t i = 0; i < getBoardSize(); i++) {
    uint64_t board_bitmask = board_[i];
    uint64_t bitmask_idx_off = i * (bits_per_entry / bits_per_tile);

    while (board_bitmask != 0) {
      // a tile will have one of its bits set, so find any bit in its bit
      // set, then divide by size (relying on remainder truncation) to find
      // its index relative to the bitmask
      uint32_t next_idx_off = __builtin_ctzl(board_bitmask) / bits_per_tile;
      uint32_t next_idx = next_idx_off + bitmask_idx_off;

      board_bitmask =
          board_bitmask & ~(tile_bitmask << (bits_per_tile * next_idx_off));

      if (!cb(toIdx(next_idx))) {
        return false;
      }
    }
  }

  return true;
}

template <uint32_t NPawns>
template <class CallbackFnT>
bool Game<NPawns>::forEachPlayablePawn(CallbackFnT cb) const {
  uint64_t turn_tile = static_cast<uint64_t>(
      state_.blackTurn ? TileState::TILE_BLACK : TileState::TILE_WHITE);

  for (uint64_t i = 0; i < getBoardSize(); i++) {
    uint64_t board_bitmask = board_[i];
    uint64_t bitmask_idx_off = i * (bits_per_entry / bits_per_tile);

    while (board_bitmask != 0) {
      // a tile will have one of its bits set, so find any bit in its bit
      // set, then divide by size (relying on remainder truncation) to find
      // its index relative to the bitmask
      uint32_t next_idx_off = __builtin_ctzl(board_bitmask) / bits_per_tile;
      uint32_t next_idx = next_idx_off + bitmask_idx_off;

      // skip this tile if it isn't our piece
      uint64_t turn_tile_mask = turn_tile << (bits_per_tile * next_idx_off);
      uint64_t clr_mask = tile_bitmask << (bits_per_tile * next_idx_off);
      if ((board_bitmask & clr_mask) != turn_tile_mask) {
        board_bitmask = board_bitmask & ~clr_mask;
        continue;
      }

      board_bitmask = board_bitmask & ~clr_mask;

      if (!cb(toIdx(next_idx))) {
        return false;
      }
    }
  }

  return true;
}
}  // namespace onoro
