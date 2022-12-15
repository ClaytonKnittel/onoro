#pragma once

#include <absl/container/flat_hash_set.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_format.h>

#include "game.h"

namespace onoro {

template <uint32_t NPawns>
class TranspositionTable {
  using TableT =
      absl::flat_hash_set<onoro::Game<NPawns>, onoro::GameHash<NPawns>,
                          onoro::GameEq<NPawns>>;
  using SymmState = typename Game<NPawns>::BoardSymmetryState;

 public:
  TranspositionTable() {}

  absl::optional<int32_t> find(const onoro::Game<NPawns>& game) {
    SymmState s = game.calcSymmetryState();
    SymmetryClassOpApplyAndReturn(s.symm_class, tryFindSymmetries, game, s);
  }

  void clear() {
    table_.clear();
  }

  void insert(const onoro::Game<NPawns>& game) {
    table_.insert(game);
  }

  void insert_or_assign(const onoro::Game<NPawns>& game) {
    auto [it, inserted] = table_.insert(game);
    if (!inserted) {
      it->setScore(game.getScore());
    }
  }

  const TableT& table() const {
    return table_;
  }

 private:
  template <class SymmetryClassOp>
  absl::optional<int32_t> tryFindSymmetries(const onoro::Game<NPawns>& game,
                                            SymmState symm_state) {
    typedef typename SymmetryClassOp::Group Group;

    // printf("%s\n", view.game().Print().c_str());
    onoro::GameView<NPawns> view(&game);

    for (bool swap_colors : { false, true }) {
      (void) swap_colors;

      for (uint32_t op_ord = 0; op_ord < Group::order(); op_ord++) {
        Group op(op_ord);
        view.setOp(op);
        // printf("hash: %s\n",
        //        GameHash<NPawns>::printC2Hash(view.hash()).c_str());

        auto it = table_.find(view);
        if (it != table_.end()) {
          /*printf("Found under %s (%s) (%s)!\n", op.toString().c_str(),
                 swap_colors ? "swapped" : "not swapped",
                 symm_state.op.toString().c_str());
          printf("\n");*/
          return it->getScore();
          /*} else {
            printf("Didn't find under %s (%s) (%s)!\n", op.toString().c_str(),
                   swap_colors ? "swapped" : "not swapped",
                   symm_state.op.toString().c_str());*/
        }
      }

      view.invertColors();
    }
    // printf("\n");

    return {};
  }

 private:
  TableT table_;
};

}  // namespace onoro
