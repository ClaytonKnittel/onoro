#pragma once

#include <utils/math/group/dihedral.h>

#include <cstdint>

#include "game.h"
#include "hash_group.h"

namespace onoro {

using namespace hash_group;

template <uint32_t NPawns>
class GameView {
 public:
  constexpr GameView(const Game<NPawns>*);
  template <class Group>
  constexpr GameView(const Game<NPawns>*, Group view_op, bool color_invert);

  GameView(const GameView& view) = default;

  // Applies the group operation to this view.
  template <class Group>
  constexpr void apply(Group op);

  // Inverts the colors in this game view.
  constexpr void invertColors();

  // The op to apply to a canonicalized view of the game.
  template <class Group>
  constexpr Group op() const;

  constexpr bool colorInvert() const;

  constexpr const Game<NPawns>& game() const;

  template <class Group>
  constexpr std::size_t hash() const;

  class PawnIterator {
   public:
    PawnIterator() {}

   private:
  };

 private:
  template <class Group>
  void setOp(Group op);

  const Game<NPawns>* game_;

  // Ordinal of the view operation to apply to the game.
  uint8_t view_op_ordinal_;
  bool color_invert_;

  static_assert(D6::order() <= UINT8_MAX);
  static_assert(D3::order() <= UINT8_MAX);
  static_assert(K4::order() <= UINT8_MAX);
  static_assert(C2::order() <= UINT8_MAX);

  // The hash of the game with op() applied.
  const std::size_t hash_;
};

template <uint32_t NPawns>
constexpr GameView<NPawns>::GameView(const Game<NPawns>* game)
    : game_(game),
      view_op_ordinal_(0),
      color_invert_(0),
      hash_(GameHash<NPawns>::calcHash(*game)) {}

template <uint32_t NPawns>
template <class Group>
constexpr GameView<NPawns>::GameView(const Game<NPawns>* game, Group view_op,
                                     bool color_invert)
    : game_(game),
      view_op_ordinal_(view_op.ordinal()),
      color_invert_(color_invert),
      hash_(GameHash<NPawns>::calcHash(*game)) {}

template <uint32_t NPawns>
template <class Group>
constexpr void GameView<NPawns>::apply(Group op) {
  setOp(op * this->op());
}

template <uint32_t NPawns>
constexpr void GameView<NPawns>::invertColors() {
  color_invert_ = !color_invert_;
}

template <uint32_t NPawns>
template <class Group>
constexpr Group GameView<NPawns>::op() const {
  return Group(view_op_ordinal_);
}

template <uint32_t NPawns>
constexpr bool GameView<NPawns>::colorInvert() const {
  return color_invert_;
}

template <uint32_t NPawns>
constexpr const Game<NPawns>& GameView<NPawns>::game() const {
  return *game_;
}

template <uint32_t NPawns>
template <class Group>
constexpr std::size_t GameView<NPawns>::hash() const {
  std::size_t h = hash_group::apply<Group>(op<Group>(), hash_);
  if (colorInvert()) {
    h = color_swap(h);
  }
  return h;
}

template <uint32_t NPawns>
template <class Group>
void GameView<NPawns>::setOp(Group op) {
  view_op_ordinal_ = op.ordinal();
}

}  // namespace onoro
