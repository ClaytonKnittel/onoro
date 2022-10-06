#pragma once

template <uint32_t N>
class DihedralEl {
 public:
  enum class Action {
    // Rotate by 2*Pi/N counterclockwise
    ROT,
    // Reflect across the x axis
    REFL
  };

  DihedralEl() : a_(Action::ROT), degree_(0) {}
  DihedralEl(Action a, uint32_t degree) : a_(a), degree_(degree) {}

  Action action() const {
    return a_;
  }

  uint32_t degree() const {
    return degree_;
  }

  std::string toString() const {
    return std::string(a_ == Action::ROT ? "r" : "s") + std::to_string(degree_);
  }

 private:
  Action a_;

  // r<n> = Rotations of 2*n*Pi/N
  // s<n> = reflection across a line n*Pi/N radians above the +x axis.
  uint32_t degree_;
};

template <uint32_t N>
DihedralEl<N> operator*(const DihedralEl<N>& e1, const DihedralEl<N>& e2) {
  switch (e1.action()) {
    case DihedralEl<N>::Action::ROT:
      switch (e2.action()) {
        case DihedralEl<N>::Action::ROT:
          return DihedralEl<N>(DihedralEl<N>::Action::ROT,
                               (e1.degree() + e2.degree()) % N);
        case DihedralEl<N>::Action::REFL:
          return DihedralEl<N>(DihedralEl<N>::Action::REFL,
                               (e1.degree() + e2.degree()) % N);
      }
    case DihedralEl<N>::Action::REFL:
      switch (e2.action()) {
        case DihedralEl<N>::Action::ROT:
          return DihedralEl<N>(DihedralEl<N>::Action::REFL,
                               (N + e1.degree() - e2.degree()) % N);
        case DihedralEl<N>::Action::REFL:
          return DihedralEl<N>(DihedralEl<N>::Action::ROT,
                               (N + e1.degree() - e2.degree()) % N);
      }
  }
}
