#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace Onoro {

class Game {
 public:
  // (x, y) coordinates as an index.
  typedef std::pair<uint32_t, uint32_t> idx_t;
  // coordinates in vector space
  typedef std::pair<uint32_t, uint32_t> pos_t;

  Game(uint32_t n_pawns);
  Game(const Game&, idx_t move);

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

  struct GameState {
    uint8_t blackTurn : 1;
    // You can play this game with a max of 8 pawns
    uint8_t pawnsInStock : 3;
    uint8_t __reserved   : 7;
  };

  uint64_t* board_;
  // Width/height of the board
  uint32_t board_len_;

  GameState state_;

  // Sum of all pos_t's of pieces on the board
  pos_t sum_of_mass_;

  // Min x/y index values of any piece
  idx_t min_idx_;
  // Max x/y index values of any piece
  idx_t max_idx_;

  /*
   * Converts an absolute index to an idx_t.
   */
  idx_t toIdx(uint32_t i) const;

  /*
   * Converts an idx_t to an absolute index.
   */
  uint32_t fromIdx(idx_t idx) const;

  /*
   * Calculates the required size of board for a game with n_pawns pawns.
   */
  static uint32_t calcBoardSize(uint32_t n_pawns);

  TileState getTile(idx_t idx) const;

  void setTile(idx_t idx, TileState);

  template <class CallbackFnT>
  void forEachNeighbor(idx_t idx, CallbackFnT cb) const {
    auto [x, y] = idx;

    if (y > 0) {
      cb({ x, y - 1 });

      if (y % 2 == 0) {
        if (x < board_len_ - 1) {
          cb({ x + 1, y - 1 });
        }
      } else {
        if (x > 0) {
          cb({ x - 1, y - 1 });
        }
      }
    }
    if (x > 0) {
      cb({ x - 1, y });
    }
    if (x < board_len_ - 1) {
      cb({ x + 1, y });
    }
    if (y < board_len_ - 1) {
      cb({ x, y + 1 });

      if (y % 2 == 0) {
        if (x < board_len_ - 1) {
          cb({ x + 1, y + 1 });
        }
      } else {
        if (x > 0) {
          cb({ x - 1, y + 1 });
        }
      }
    }
  }

  template <class CallbackFnT>
  void forEachMove(CallbackFnT cb) const {
    static constexpr uint32_t tmp_board_tile_bits = 2;
    static constexpr uint64_t tmp_board_tile_mask =
        (uint64_t(1) << tmp_board_tile_bits) - 1;

    uint64_t tmp_board_len =
        (board_len_ * board_len_ * tmp_board_tile_bits + bits_per_entry - 1) /
        bits_per_entry;
    // bitvector of moves already taken
    uint64_t tmp_board[tmp_board_len];
    memset(tmp_board, 0, tmp_board_len * 8);

    for (uint64_t i = 0; i < tmp_board_len; i++) {
      uint64_t board_bitmask = board_[i];
      uint64_t bitmask_idx_off = i * (bits_per_entry / bits_per_tile);

      while (board_bitmask != 0) {
        // a tile will have one of its bits set, so find any bit in its bit set,
        // then divide by size (relying on remainder truncation) to find its
        // index relative to the bitmask
        uint32_t next_idx_off = __builtin_ctzl(board_bitmask) / bits_per_tile;
        uint32_t next_idx = next_idx_off + bitmask_idx_off;
        board_bitmask =
            board_bitmask & ~(tile_bitmask << (bits_per_tile * next_idx));

        forEachNeighbor(toIdx(next_idx), [&tmp_board, this,
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
                cb(neighbor_idx);
              }
            }
          }
        });
      }
    }
  }
};
}  // namespace Onoro
