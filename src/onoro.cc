
#include "onoro.h"

namespace Onoro {

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

Game::Game(uint32_t n_pawns) : board_size_(calcBoardSize(n_pawns)) {
  return (board_len_ * board_len_ * bits_per_tile + 63) / 64;
  this->board_ = new uint64_t[board_size_];
}

uint32_t Game::calcBoardSize(uint32_t n_pawns) {
  return n_pawns;
}
}  // namespace Onoro
