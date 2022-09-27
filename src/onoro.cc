
#include "onoro.h"

#include <sstream>

namespace Onoro {

Game::idx_t operator+(const Game::idx_t& a, const Game::idx_t& b) {
  return { a.first + b.first, a.second + b.second };
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
Game::Game(uint32_t n_pawns)
    : board_len_(calcBoardSize(n_pawns)),
      state_({ 0, static_cast<uint8_t>(n_pawns), 0 }) {
  if (n_pawns > 2 * max_pawns_per_player) {
    throw new std::runtime_error("Cannot play with more than 16 pawns");
  }

  uint64_t board_size =
      (board_len_ * board_len_ * bits_per_tile + bits_per_entry - 1) /
      bits_per_entry;
  this->board_ = new uint64_t[board_size]();

  uint32_t mid_idx = (board_len_ - 1) / 2;
  idx_t b_start = { mid_idx, mid_idx };
  idx_t w_start = { mid_idx, mid_idx + 1 };
  idx_t b_next = { mid_idx + 1, mid_idx };

  setTile(b_start, TileState::TILE_BLACK);
  setTile(w_start, TileState::TILE_WHITE);
  setTile(b_next, TileState::TILE_BLACK);

  min_idx_ = b_start;
  max_idx_ = { mid_idx + 1, mid_idx + 1 };
  sum_of_mass_ = b_start + w_start + b_next;

  printf("%s\n", this->Print().c_str());

  forEachMove([this](idx_t idx) {
    Game g2(*this, idx);
    printf("Move: (%u, %u)\n", idx.first, idx.second);
    printf("%s\n", g2.Print().c_str());

    g2.forEachMove([&g2](idx_t idx2) {
      Game g3(g2, idx2);
      printf("2nd move: (%u, %u)\n", idx2.first, idx2.second);
      printf("%s\n", g3.Print().c_str());
    });

    printf("\n\n");
  });
}

Game::Game(const Game& g, idx_t move)
    : board_len_(g.board_len_),
      state_(
          { !g.state_.blackTurn,
            static_cast<uint8_t>(g.state_.pawnsInStock - !g.state_.blackTurn),
            g.state_.__reserved }) {
  uint64_t board_size =
      (board_len_ * board_len_ * bits_per_tile + bits_per_entry - 1) /
      bits_per_entry;
  board_ = new uint64_t[board_size];
  memcpy(board_, g.board_, board_size * sizeof(uint64_t));

  setTile(move,
          g.state_.blackTurn ? TileState::TILE_BLACK : TileState::TILE_WHITE);
}

std::string Game::Print() const {
  static const char tile_str[3] = {
    '.',
    'B',
    'W',
  };

  std::ostringstream ostr;
  for (uint32_t y = 0; y < board_len_; y++) {
    if (y % 2 == 0) {
      ostr << " ";
    }

    for (uint32_t x = 0; x < board_len_; x++) {
      ostr << tile_str[static_cast<int>(getTile({ x, y }))];
      if (x < board_len_ - 1) {
        ostr << " ";
      }
    }

    if (y < board_len_ - 1) {
      ostr << "\n";
    }
  }
  return ostr.str();
}

Game::idx_t Game::toIdx(uint32_t i) const {
  return { i % board_len_, i / board_len_ };
}

uint32_t Game::fromIdx(Game::idx_t idx) const {
  return idx.first + idx.second * board_len_;
}

uint32_t Game::calcBoardSize(uint32_t n_pawns) {
  return n_pawns;
}

Game::TileState Game::getTile(idx_t idx) const {
  auto [x, y] = idx;
  uint32_t i = x + y * board_len_;

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);

  return TileState((tile_bitv >> (bits_per_tile * bitv_idx)) & tile_bitmask);
}

void Game::setTile(idx_t idx, TileState piece) {
  auto [x, y] = idx;
  uint32_t i = x + y * board_len_;

  uint64_t tile_bitv = board_[i * bits_per_tile / bits_per_entry];
  uint32_t bitv_idx = i % (bits_per_entry / bits_per_tile);
  uint64_t bitv_mask =
      (static_cast<uint64_t>(piece) << (bits_per_tile * bitv_idx));

  board_[i * bits_per_tile / bits_per_entry] = tile_bitv | bitv_mask;
}
}  // namespace Onoro
