#pragma once

#include <utility>

#include "onoro.h"
#include "union_find.h"

namespace Onoro {

class SimpleGame {
 private:
  /*
   * Calculates the required size of board for a game with n_pawns pawns.
   */
  uint32_t getBoardLen();

  /*
   * Calculates the number of tiles in the board.
   */
  uint32_t getBoardNumTiles();

 public:
  SimpleGame();
  SimpleGame(const SimpleGame&) = delete;
  SimpleGame(SimpleGame&&) = default;

  SimpleGame& operator=(SimpleGame&&) = default;

  SimpleGame(const SimpleGame&, idx_t move);
  SimpleGame(const SimpleGame&, idx_t move, idx_t from);

 private:
  enum class TileState = {
    TILE_EMPTY,
    TILE_BLACK,
    TILE_WHITE,
  };

  static constexpr uint32_t min_neighbors_per_pawn = 2;
  static constexpr uint32_t n_in_row_to_win = 4;

  uint32_t n_pawns_;
  uint32_t turn_;
  std::vector<TileState> board_;
  bool gameOver;

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

  template <class CallbackFnT>
  void forEachMove(CallbackFnT cb) const;

  // calls cb with arguments to : idx_t, from : idx_t
  template <class CallbackFnT>
  void forEachMoveP2(CallbackFnT cb) const;
};

uint32_t Game<NPawns>::getBoardLen() {
  return n_pawns_ * 2 + 2;
}

uint32_t Game<NPawns>::getBoardNumTiles() {
  return getBoardLen() * getBoardLen();
}

}  // namespace Onoro
