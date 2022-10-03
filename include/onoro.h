#pragma once

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include "union_find.h"

namespace Onoro {

struct pos_t {
  int32_t x, y;
};

// (x, y) coordinates as an index.
typedef std::pair<uint32_t, uint32_t> idx_t;
// coordinates in vector space
// typedef std::pair<int32_t, int32_t> pos_t;

template <uint32_t NPawns>
class Game;

template <uint32_t NPawns>
class GameHash {
 public:
  GameHash();

  std::size_t operator()(const Game<NPawns>& g) const noexcept;
};

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

  // copies src into dst, shifting the bits in
  // src left by "bit_offset" (with overflow propagated)
  // bit_offset may be negative and any size (so long as its absolute value is <
  // 64 * n_8bytes)
  static void copyAndShift(uint64_t* __restrict__ dst,
                           const uint64_t* __restrict__ src, size_t n_8bytes,
                           int32_t bit_offset);

  static std::pair<int, pos_t> calcMoveShiftAndOffset(const Game&, idx_t move);

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

 private:
  enum class TileState {
    TILE_EMPTY = 0,
    TILE_BLACK = 1,
    TILE_WHITE = 2,
  };
  // bits per entry in the board
  static constexpr uint32_t bits_per_entry = 64;
  static constexpr uint32_t bits_per_tile = 2;
  static constexpr uint64_t tile_bitmask = (1 << bits_per_tile) - 1;
  static constexpr uint32_t max_pawns_per_player = 8;
  static constexpr uint32_t min_neighbors_per_pawn = 2;
  static constexpr uint32_t n_in_row_to_win = 4;

  struct GameState {
    // You can play this game with a max of 8 pawns, and turn count stops
    // incrementing after the end of phase 1
    uint8_t turn       : 4;
    uint8_t blackTurn  : 1;
    uint8_t finished   : 1;
    uint8_t __reserved : 2;
  };

  uint64_t board_[getBoardSize()];

  GameState state_;

  // Sum of all pos_t's of pieces on the board
  pos_t sum_of_mass_;

  // Min x/y index values of any piece
  idx_t min_idx_;
  // Max x/y index values of any piece
  idx_t max_idx_;

 public:
  /*
   * Converts an absolute index to an idx_t.
   */
  idx_t toIdx(uint32_t i) const;

  /*
   * Converts an idx_t to an absolute index.
   */
  uint32_t fromIdx(idx_t idx) const;

  pos_t idxToPos(idx_t idx) const;

  idx_t posToIdx(pos_t pos) const;

  bool inBounds(idx_t idx) const;

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
  void forEachMove(CallbackFnT cb) const;

  // calls cb with arguments to : idx_t, from : idx_t
  template <class CallbackFnT>
  void forEachMoveP2(CallbackFnT cb) const;

 private:
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

bool operator==(const pos_t& a, const pos_t& b) {
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const pos_t& a, const pos_t& b) {
  return a.x != b.x || a.y != b.y;
}

template <typename T>
pos_t operator*(const T& a, const pos_t& b) {
  return { static_cast<int32_t>(a * b.x), static_cast<int32_t>(a * b.y) };
}

pos_t operator+(const pos_t& a, const pos_t& b) {
  return { a.x + b.x, a.y + b.y };
}

pos_t operator+=(pos_t& a, const pos_t& b) {
  a = { a.x + b.x, a.y + b.y };
  return a;
}

pos_t operator-(const pos_t& a, const pos_t& b) {
  return { a.x - b.x, a.y - b.y };
}

pos_t operator-=(pos_t& a, const pos_t& b) {
  a = { a.x - b.x, a.y - b.y };
  return a;
}

template <uint32_t NPawns>
GameHash<NPawns>::GameHash() {
  printf("Constructed!\n");
}

template <uint32_t NPawns>
std::size_t GameHash<NPawns>::operator()(const Game<NPawns>& g) const noexcept {
  return 0;
}

template <uint32_t NPawns>
bool GameEq<NPawns>::operator()(const Game<NPawns>& g1,
                                const Game<NPawns>& g2) const noexcept {
  return true;
}

template <uint32_t NPawns>
constexpr uint32_t Game<NPawns>::getBoardLen() {
  return NPawns * 2 + 2;
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
std::pair<int, pos_t> Game<NPawns>::calcMoveShiftAndOffset(const Game& g,
                                                           idx_t move) {
  int shift = 0;
  pos_t offset = { 0, 0 };
  if (move.second == 0 && g.max_idx_.second < getBoardLen() - 1) {
    shift = getBoardLen() * 2;
    offset.x = 1;
    offset.y = 2;
  } else if (move.second == getBoardLen() - 1 && g.max_idx_.second > 0) {
    shift = -static_cast<int>(getBoardLen() * 2);
    offset.x = -1;
    offset.y = -2;
  }
  if (move.first == 0 && g.max_idx_.first < getBoardLen() - 1) {
    shift++;
    offset.x++;
  } else if (move.first == getBoardLen() - 1 && g.max_idx_.first > 0) {
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

  uint32_t mid_idx = (getBoardLen() - 1) / 2;
  idx_t b_start = { mid_idx, mid_idx };
  idx_t w_start = { mid_idx + !(mid_idx & 1), mid_idx + 1 };
  idx_t b_next = { mid_idx + 1, mid_idx };

  setTile(b_start, TileState::TILE_BLACK);
  setTile(w_start, TileState::TILE_WHITE);
  setTile(b_next, TileState::TILE_BLACK);

  min_idx_ = b_start;
  max_idx_ = { mid_idx + 1, mid_idx + 1 };
  sum_of_mass_ = idxToPos(b_start) + idxToPos(w_start) + idxToPos(b_next);
}

template <uint32_t NPawns>
Game<NPawns>::Game(const Game<NPawns>& g, idx_t move)
    : state_({ static_cast<uint8_t>(g.state_.turn + 1), !g.state_.blackTurn, 0,
               0 }) {
  auto [shift, offset] = calcMoveShiftAndOffset(g, move);
  copyAndShift(reinterpret_cast<uint64_t*>(board_),
               reinterpret_cast<const uint64_t*>(g.board_), getBoardSize(),
               shift * bits_per_tile);

  min_idx_ = { std::min(move.first, g.min_idx_.first),
               std::min(move.second, g.min_idx_.second) };
  max_idx_ = { std::max(move.first, g.max_idx_.first),
               std::max(move.second, g.max_idx_.second) };
  min_idx_ = posToIdx(idxToPos(min_idx_) + offset);
  max_idx_ = posToIdx(idxToPos(max_idx_) + offset);
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

  min_idx_ = { std::min(move.first, g.min_idx_.first),
               std::min(move.second, g.min_idx_.second) };
  max_idx_ = { std::max(move.first, g.max_idx_.first),
               std::max(move.second, g.max_idx_.second) };
  min_idx_ = posToIdx(idxToPos(min_idx_) + offset);
  max_idx_ = posToIdx(idxToPos(max_idx_) + offset);
  sum_of_mass_ =
      g.sum_of_mass_ + idxToPos(move) - idxToPos(from) + (NPawns * 2) * offset;

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
      auto [mx, my] = posToIdx(sum_of_mass_);
      mx = (mx + nPawnsInPlay() / 2) / nPawnsInPlay();
      my = (my + nPawnsInPlay() / 2) / nPawnsInPlay();

      if (x == mx && y == my) {
        ostr << "\033[0;32m";
      }
      ostr << tile_str[static_cast<int>(getTile({ x, y }))];
      if (x == mx && y == my) {
        ostr << "\033[0;39m";
      }
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
idx_t Game<NPawns>::toIdx(uint32_t i) const {
  return { i % getBoardLen(), i / getBoardLen() };
}

template <uint32_t NPawns>
uint32_t Game<NPawns>::fromIdx(idx_t idx) const {
  return idx.first + idx.second * getBoardLen();
}

template <uint32_t NPawns>
pos_t Game<NPawns>::idxToPos(idx_t idx) const {
  return { static_cast<int32_t>(idx.first + idx.second / 2),
           static_cast<int32_t>(idx.second) };
}

template <uint32_t NPawns>
idx_t Game<NPawns>::posToIdx(pos_t pos) const {
  return { static_cast<uint32_t>(pos.x - static_cast<uint32_t>(pos.y) / 2),
           static_cast<uint32_t>(pos.y) };
}

template <uint32_t NPawns>
bool Game<NPawns>::inBounds(idx_t idx) const {
  auto [x, y] = idx;
  return x >= 0 && x < getBoardLen() && y >= 0 && y < getBoardLen();
}

template <uint32_t NPawns>
uint32_t Game<NPawns>::nPawnsInPlay() const {
  return state_.turn + 1;
}

template <uint32_t NPawns>
bool Game<NPawns>::inPhase2() const {
  return state_.turn == NPawns * 2 - 1;
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
  uint32_t i = x + y * getBoardLen();

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
  auto [x, y] = idx;
  uint32_t i = x + y * getBoardLen();

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);
  uint64_t bitv_mask =
      (static_cast<uint64_t>(piece) << (bits_per_tile * bitv_idx));

  board_[i * bits_per_tile / bits_per_entry] = tile_bitv | bitv_mask;
}

template <uint32_t NPawns>
void Game<NPawns>::clearTile(idx_t idx) {
  auto [x, y] = idx;
  uint32_t i = x + y * getBoardLen();

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
  pos_t last_move_pos = idxToPos(last_move);
  uint32_t n_in_row;

  TileState move_color = getTile(last_move);

  {
    n_in_row = 0;
    pos_t last_pos = last_move_pos + (pos_t){ n_in_row_to_win + 1, 0 };
    for (pos_t i = last_move_pos - (pos_t){ n_in_row_to_win, 0 }; i != last_pos;
         i += (pos_t){ 1, 0 }) {
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
    pos_t last_pos =
        last_move_pos + (pos_t){ n_in_row_to_win + 1, n_in_row_to_win + 1 };
    for (pos_t i = last_move_pos - (pos_t){ n_in_row_to_win, n_in_row_to_win };
         i != last_pos; i += (pos_t){ 1, 1 }) {
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
    pos_t last_pos = last_move_pos + (pos_t){ 0, n_in_row_to_win + 1 };
    for (pos_t i = last_move_pos - (pos_t){ 0, n_in_row_to_win }; i != last_pos;
         i += (pos_t){ 0, 1 }) {
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

    if (y % 2 == 0) {
      if (x < getBoardLen() - 1) {
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
  if (x < getBoardLen() - 1) {
    CB_OR_RET({ x + 1, y });
  }
  if (y < getBoardLen() - 1) {
    CB_OR_RET({ x, y + 1 });

    if (y % 2 == 0) {
      if (x < getBoardLen() - 1) {
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

    if (y % 2 == 0) {
      if (x < getBoardLen() - 1) {
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
void Game<NPawns>::forEachMove(CallbackFnT cb) const {
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

  forEachPawn([this, &tmp_board, &cb](idx_t next_idx) {
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
  /*
  for (uint64_t i = 0; i < tmp_board_len; i++) {
    uint64_t board_bitmask = board_[i];
    uint64_t bitmask_idx_off = i * (bits_per_entry / bits_per_tile);

    while (board_bitmask != 0) {
      // a tile will have one of its bits set, so find any bit in its bit
      // set, then divide by size (relying on remainder truncation) to find
      // its index relative to the bitmask
      uint32_t next_idx_off = __builtin_ctzl(board_bitmask) / bits_per_tile;
      uint32_t next_idx = next_idx_off + bitmask_idx_off;
      board_bitmask =
          board_bitmask & ~(tile_bitmask << (bits_per_tile * next_idx));

      bool res = forEachNeighbor(toIdx(next_idx), [&tmp_board, this,
                                                   &cb](idx_t neighbor_idx) {
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

      if (!res) {
        return;
      }
    }
  }
  */
}

template <uint32_t NPawns>
template <class CallbackFnT>
void Game<NPawns>::forEachMoveP2(CallbackFnT cb) const {
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
    bool res = forEachNeighbor(next_idx, [&tmp_board,
                                          this](idx_t neighbor_idx) {
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

    return res;
  });
  /*
  for (uint64_t i = 0; i < tmp_board_len; i++) {
    uint64_t board_bitmask = board_[i];
    uint64_t bitmask_idx_off = i * (bits_per_entry / bits_per_tile);

    while (board_bitmask != 0) {
      // a tile will have one of its bits set, so find any bit in its bit
      // set, then divide by size (relying on remainder truncation) to find
      // its index relative to the bitmask
      uint32_t next_idx_off = __builtin_ctzl(board_bitmask) / bits_per_tile;
      uint32_t next_idx = next_idx_off + bitmask_idx_off;
      board_bitmask = board_bitmask &
                      ~(tmp_board_tile_mask << (bits_per_tile * next_idx_off));

      bool res = forEachNeighbor(toIdx(next_idx), [&tmp_board,
                                                   this](idx_t neighbor_idx) {
        uint32_t idx = fromIdx(neighbor_idx);
        uint32_t tb_shift = tmp_board_tile_bits *
                            (idx % (bits_per_entry / tmp_board_tile_bits));
        uint64_t tbb = tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)];
        uint64_t mask = tmp_board_tile_mask << tb_shift;
        uint64_t full_mask = uint64_t(min_neighbors_per_pawn + 1) << tb_shift;

        if ((tbb & mask) != full_mask) {
          tbb += (uint64_t(1) << tb_shift);
          tmp_board[idx / (bits_per_entry / tmp_board_tile_bits)] = tbb;
        }

        return true;
      });

      if (!res) {
        return;
      }
    }
  }
  */

  // Another pass to enumerate all moves
  forEachPlayablePawn([this, &tmp_board, &cb](idx_t next_idx) {
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
    forEachNeighbor(next_idx, [&tmp_board, this](idx_t neighbor_idx) {
      uint32_t idx = fromIdx(neighbor_idx);
      uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
      uint32_t tb_shift =
          tmp_board_tile_bits * (idx % (bits_per_entry / tmp_board_tile_bits));

      tmp_board[tb_idx] += (uint64_t(1) << tb_shift);
      return true;
    });

    return true;
  });
  /*
  for (uint64_t i = 0; i < tmp_board_len; i++) {
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
      uint64_t clr_mask = tmp_board_tile_mask << (bits_per_tile * next_idx_off);
      if ((board_bitmask & clr_mask) != turn_tile_mask) {
        board_bitmask = board_bitmask & ~clr_mask;
        continue;
      }

      board_bitmask = board_bitmask & ~clr_mask;

      // TODO
      // UnionFind<uint32_t> uf(getBoardNumTiles() + 1);

      // number of neighbors with 1 neighbor after removing this piece
      uint32_t n_to_satisfy = 0;
      // decrease neighbor count of all neighbors
      forEachNeighbor(toIdx(next_idx), [&tmp_board, &n_to_satisfy,
                                        this](idx_t neighbor_idx) {
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
        uint64_t tmp_bitmask_idx_off =
            j * (bits_per_entry / tmp_board_tile_bits);

        while (tmp_board_bitmask != 0) {
          uint32_t tmp_next_idx_off =
              __builtin_ctzl(tmp_board_bitmask) / tmp_board_tile_bits;
          uint32_t tb_shift = tmp_next_idx_off * tmp_board_tile_bits;
          uint32_t tmp_next_idx = tmp_next_idx_off + tmp_bitmask_idx_off;
          uint64_t clr_mask2 = tmp_board_tile_mask << tb_shift;

          // skip this tile if it isn't empty (this will also skip the piece's
          // old location since we haven't removed it, which we want)
          if (getTile(toIdx(tmp_next_idx)) != TileState::TILE_EMPTY ||
              ((tmp_board_bitmask >> tb_shift) & tmp_board_tile_mask) <= 1) {
            tmp_board_bitmask = tmp_board_bitmask & ~clr_mask2;
            continue;
          }

          tmp_board_bitmask = tmp_board_bitmask & ~clr_mask2;

          uint32_t n_satisfied = 0;
          forEachNeighbor(toIdx(tmp_next_idx), [&tmp_board, &n_satisfied,
                                                this](idx_t neighbor_idx) {
            uint32_t idx = fromIdx(neighbor_idx);
            uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
            uint32_t tb_shift = tmp_board_tile_bits *
                                (idx % (bits_per_entry / tmp_board_tile_bits));

            if (((tmp_board[tb_idx] & tmp_board_tile_mask) >> tb_shift) == 1) {
              n_satisfied++;
            }
            return true;
          });

          if (n_satisfied == n_to_satisfy) {
            if (!cb(toIdx(tmp_next_idx), toIdx(next_idx))) {
              return;
            }
          }
        }
      }

      // increase neighbor count of all neighbors
      forEachNeighbor(toIdx(next_idx), [&tmp_board, this](idx_t neighbor_idx) {
        uint32_t idx = fromIdx(neighbor_idx);
        uint32_t tb_idx = idx / (bits_per_entry / tmp_board_tile_bits);
        uint32_t tb_shift = tmp_board_tile_bits *
                            (idx % (bits_per_entry / tmp_board_tile_bits));

        tmp_board[tb_idx] += (uint64_t(1) << tb_shift);
        return true;
      });
    }
  }
  */
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
}  // namespace Onoro
