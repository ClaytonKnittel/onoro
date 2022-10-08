#pragma once

template <uint32_t N>
class DihedralEl {
  template <uint32_t N_>
  friend DihedralEl<N_> operator*(const DihedralEl<N_>& e1,
                                  const DihedralEl<N_>& e2);

 public:
  // An element_t is equal to <n> + (rfl ? N : 0), where <n> is the number of
  // rotations applied (0 to N-1), and rfl is true if a reflection was
  // applied.
  typedef uint8_t element_t;

  enum class Action : uint64_t {
    // Rotate by 2*Pi/N counterclockwise
    ROT = 0,
    // Reflect across the x axis
    REFL,
  };

  static constexpr Action action(element_t e) {
    return static_cast<Action>(e / N);
  }

  static constexpr uint32_t degree(element_t e) {
    return static_cast<uint32_t>(e % N);
  }

  constexpr DihedralEl() : DihedralEl(Action::ROT, 0) {}

  // r<n> = Rotations of 2*n*Pi/N
  // s<n> = reflection across a line n*Pi/N radians above the +x axis.
  constexpr DihedralEl(Action a, uint32_t degree)
      : DihedralEl(
            static_cast<uint8_t>(static_cast<uint32_t>(a) * N + degree)) {}

  explicit constexpr DihedralEl(element_t e) : e_(e) {}

  /*
   * Returns a unique uint32_t for this element, with the guarantee that it will
   * be the same when called on any equivalent elements, and that the ordinals
   * of all elements are >= 0 and contiguous.
   */
  constexpr uint32_t ordinal() const {
    return e_;
  }

  Action action() const {
    return action(e_);
  }

  uint32_t degree() const {
    return degree(e_);
  }

  std::string toString() const {
    return std::string(action() == Action::ROT ? "r" : "s") +
           std::to_string(degree());
  }

 private:
  static constexpr std::array<element_t, 4 * N * N> genTable() {
    std::array<element_t, 4 * N * N> table{ 0 };

    for (element_t i = 0; i < static_cast<element_t>(2 * N); i++) {
      for (element_t j = 0; j < static_cast<element_t>(2 * N); j++) {
        // calculates the effect of the operation e(i) * e(j)
        element_t& el = table[i * (2 * N) + j];

        switch (action(i)) {
          case DihedralEl<N>::Action::ROT: {
            switch (action(j)) {
              case DihedralEl<N>::Action::ROT: {
                el = DihedralEl<N>(DihedralEl<N>::Action::ROT,
                                   (degree(i) + degree(j)) % N)
                         .e_;
                break;
              }
              case DihedralEl<N>::Action::REFL: {
                el = DihedralEl<N>(DihedralEl<N>::Action::REFL,
                                   (degree(i) + degree(j)) % N)
                         .e_;
                break;
              }
            }
            break;
          }
          case DihedralEl<N>::Action::REFL: {
            switch (action(j)) {
              case DihedralEl<N>::Action::ROT: {
                el = DihedralEl<N>(DihedralEl<N>::Action::REFL,
                                   (N + degree(i) - degree(j)) % N)
                         .e_;
                break;
              }
              case DihedralEl<N>::Action::REFL: {
                el = DihedralEl<N>(DihedralEl<N>::Action::ROT,
                                   (N + degree(i) - degree(j)) % N)
                         .e_;
                break;
              }
            }
            break;
          }
        }
      }
    }

    return table;
  }

 private:
  element_t e_;

  static constexpr std::array<element_t, 4 * N* N> compose_table = genTable();
};

template <uint32_t N>
DihedralEl<N> operator*(const DihedralEl<N>& e1, const DihedralEl<N>& e2) {
  return DihedralEl<N>(DihedralEl<N>::compose_table[e1.e_ * (2 * N) + e2.e_]);
}
