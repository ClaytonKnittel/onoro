
#include <cinttypes>

#include "onoro.h"

using namespace onoro;

template <class Group>
static bool test_invariant(Group op, std::size_t hash);

template <>
bool test_invariant<D6>(D6 op, std::size_t hash) {
  switch (op.ordinal()) {
    case D6(D6::Action::ROT, 1).ordinal():
    case D6(D6::Action::REFL, 0).ordinal():
    case D6(D6::Action::REFL, 1).ordinal():
    case D6(D6::Action::REFL, 2).ordinal():
    case D6(D6::Action::REFL, 3).ordinal():
    case D6(D6::Action::REFL, 4).ordinal():
    case D6(D6::Action::REFL, 5).ordinal(): {
      std::size_t h_inv = make_invariant_d6(op, hash);
      if (apply_d6(op, h_inv) != h_inv) {
        fprintf(stderr,
                "Invariant D6 hash varies: %016" PRIx64 " vs %016" PRIx64 "\n",
                h_inv, apply_d6(op, h_inv));
        return false;
      }
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D6(D6::Action::ROT, 0).ordinal():
    case D6(D6::Action::ROT, 2).ordinal():
    case D6(D6::Action::ROT, 3).ordinal():
    case D6(D6::Action::ROT, 4).ordinal():
    case D6(D6::Action::ROT, 5).ordinal(): {
      return true;
    }

    default: {
      fprintf(stderr, "Unknown ordinal %d\n", op.ordinal());
      return false;
    }
  }
}

template <>
bool test_invariant<D3>(D3 op, std::size_t hash) {
  switch (op.ordinal()) {
    case D3(D3::Action::ROT, 1).ordinal():
    case D3(D3::Action::REFL, 0).ordinal():
    case D3(D3::Action::REFL, 1).ordinal():
    case D3(D3::Action::REFL, 2).ordinal(): {
      std::size_t h_inv = make_invariant_d3(op, hash);
      if (apply_d3(op, h_inv) != h_inv) {
        fprintf(stderr,
                "Invariant D3 hash varies: %016" PRIx64 " vs %016" PRIx64 "\n",
                h_inv, apply_d3(op, h_inv));
        return false;
      }
    }

    // Don't support making invariant under rotations other than the basic
    // rotation.
    case D3(D3::Action::ROT, 0).ordinal():
    case D3(D3::Action::ROT, 2).ordinal(): {
      return true;
    }

    default: {
      fprintf(stderr, "Unknown ordinal %d\n", op.ordinal());
      return false;
    }
  }
}

template <>
bool test_invariant<K4>(K4 op, std::size_t hash) {
  switch (op.ordinal()) {
    case K4(C2(1), C2(0)).ordinal():
    case K4(C2(0), C2(1)).ordinal():
    case K4(C2(1), C2(1)).ordinal(): {
      std::size_t h_inv = make_invariant_k4(op, hash);
      if (apply_k4(op, h_inv) != h_inv) {
        fprintf(stderr,
                "Invariant K4 hash varies: %016" PRIx64 " vs %016" PRIx64 "\n",
                h_inv, apply_k4(op, h_inv));
        return false;
      }
    }

    // Don't support making invariant under identity.
    case K4(C2(0), C2(0)).ordinal(): {
      return true;
    }

    default: {
      fprintf(stderr, "Unknown ordinal %d\n", op.ordinal());
      return false;
    }
  }
}

template <>
bool test_invariant<C2>(C2 op, std::size_t hash) {
  switch (op.ordinal()) {
    case C2(1).ordinal(): {
      std::size_t h_inv = make_invariant_c2(op, hash);
      if (apply_c2(op, h_inv) != h_inv) {
        fprintf(stderr,
                "Invariant C2 hash varies: %016" PRIx64 " vs %016" PRIx64 "\n",
                h_inv, apply_c2(op, h_inv));
        return false;
      }
    }

    // Don't support making invariant under identity.
    case C2(0).ordinal(): {
      return true;
    }

    default: {
      fprintf(stderr, "Unknown ordinal %d\n", op.ordinal());
      return false;
    }
  }
}

template <class Group>
static bool test_group() {
  std::size_t h = (UINT64_C(0x123) << 0) | (UINT64_C(0x245) << 10) |
                  (UINT64_C(0x367) << 20) | (UINT64_C(0x089) << 30) |
                  (UINT64_C(0x1ab) << 40) | (UINT64_C(0x2cd) << 50);

  for (uint32_t o_a = 0; o_a < Group::order(); o_a++) {
    Group a(o_a);

    if (!test_invariant<Group>(a, h)) {
      return false;
    }

    for (uint32_t o_b = 0; o_b < Group::order(); o_b++) {
      Group b(o_b);
      std::size_t h_b = hash_group::apply<Group>(b, h);

      Group c = a * b;
      std::size_t h_c = apply<Group>(c, h);

      if (apply<Group>(a, h_b) != h_c) {
        fprintf(stderr,
                "Hashes not equal:\n0x%016" PRIx64 "\n0x%016" PRIx64 "\n",
                apply<Group>(a, h_b), h_c);
        return false;
      }
    }
  }

  return true;
}

int main() {
  if (!test_group<D6>()) {
    return -1;
  }
  if (!test_group<D3>()) {
    return -1;
  }
  if (!test_group<K4>()) {
    return -1;
  }
  if (!test_group<C2>()) {
    return -1;
  }
  return 0;
}
