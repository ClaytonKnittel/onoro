#pragma once

#include <cstdint>

namespace Onoro {

class Game {
 public:
  Game(uint32_t n_pawns);

 private:
  enum class TileState {
    TILE_EMPTY = 0,
    TILE_BLACK = 1,
    TILE_WHITE = 2,
  };
  static constexpr uint32_t bits_per_tile = 2;

  uint64_t* board_;
  // Width/height of the board
  uint32_t board_len_;

  /*
   * Calculates the required size of board for a game with n_pawns pawns.
   */
  static uint32_t calcBoardSize(uint32_t n_pawns);

  template <class CallbackFnT>
  void forEachNeighbor(uint32_t idx, CallbackFnT cb) {
    uint32_t
  }
};
}  // namespace Onoro
