#pragma once

#include <utils/math/group/dihedral.h>

#include <cstdint>

#include "game.h"

namespace onoro {

namespace {
using namespace util::math::group;

typedef Dihedral<6> D6;
typedef Dihedral<3> D3;
typedef DirectProduct<Cyclic<2>, Cyclic<2>> K4;
typedef Cyclic<2> C2;

}  // namespace

template <uint32_t NPawns>
class GameView {
 public:
  constexpr GameView(const Game<NPawns>*);
  template <class Group>
  constexpr GameView(const Game<NPawns>*, Group view_op);

  GameView(const GameView& view) = default;

  // The op to apply to a canonicalized view of the game.
  template <class Group>
  constexpr Group op() const;

  constexpr const Game<NPawns>& game() const;

 private:
  static constexpr std::intptr_t bundle(const Game<NPawns>* game,
                                        uint32_t ordinal);
  static constexpr std::intptr_t OP_MASK = 0xf;

  static_assert(D6::order() <= OP_MASK);
  static_assert(D3::order() <= OP_MASK);
  static_assert(K4::order() <= OP_MASK);
  static_assert(C2::order() <= OP_MASK);

  // First 4 bits reserved for the symmetry operation, remaining bits reserved
  // for the pointer to the game object. Requires pointers to be aligned by 16
  // bytes.
  const std::intptr_t game_op_;

  // The hash of the game with op() applied.
  const std::size_t hash_;
};

template <uint32_t NPawns>
constexpr GameView<NPawns>::GameView(const Game<NPawns>* game)
    : game_op_(GameView<NPawns>::bundle(game, 0)), hash_(0) {
  if ((reinterpret_cast<std::intptr_t>(game) & OP_MASK) != 0) {
    throw new std::runtime_error("Expected ptr to be aligned by 16 bytes");
  }
}

template <uint32_t NPawns>
template <class Group>
constexpr GameView<NPawns>::GameView(const Game<NPawns>* game, Group view_op)
    : game_op_(GameView<NPawns>::bundle(game, view_op.ordinal())), hash_(0) {
  if ((reinterpret_cast<std::intptr_t>(game) & OP_MASK) != 0) {
    throw new std::runtime_error("Expected ptr to be aligned by 16 bytes");
  }
}

template <uint32_t NPawns>
template <class Group>
constexpr Group GameView<NPawns>::op() const {
  return Group(static_cast<uint32_t>(game_op_ & OP_MASK));
}

template <uint32_t NPawns>
constexpr const Game<NPawns>& GameView<NPawns>::game() const {
  return *reinterpret_cast<const Game<NPawns>*>(game_op_ & ~OP_MASK);
}

template <uint32_t NPawns>
constexpr std::intptr_t GameView<NPawns>::bundle(const Game<NPawns>* game,
                                                 uint32_t ordinal) {
  return reinterpret_cast<std::intptr_t>(game) |
         static_cast<std::intptr_t>(ordinal);
}

}  // namespace onoro
