#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <absl/status/statusor.h>

#include <cstdio>

#include "game_state.pb.h"
#include "onoro.h"

static constexpr uint32_t NPawns = 16;

template <class SymmetryClassOp>
static absl::StatusOr<bool> _doEqUnderSymmetries(onoro::Game<NPawns> game1,
                                                 onoro::Game<NPawns> game2) {
  typedef typename SymmetryClassOp::Group Group;
  using SymmState = typename onoro::Game<NPawns>::BoardSymmetryState;

  onoro::GameEq<NPawns> games_eq;
  onoro::GameHash<NPawns> game_hash;

  onoro::GameView<NPawns> view1(&game1);

  std::size_t h1 = game_hash(game1);
  std::size_t h2 = game_hash(game2);

  for (bool swap_colors : { false, true }) {
    (void) swap_colors;

    for (uint32_t op_ord = 0; op_ord < Group::order(); op_ord++) {
      Group op(op_ord);
      view1.setOp(op);

      if (games_eq(view1, game2)) {
        std::size_t h1_rot = onoro::hash_group::apply(view1.op<Group>(), h1);
        if (swap_colors) {
          h1_rot = onoro::hash_group::color_swap(h1_rot);
        }
        if (h1_rot != view1.hash()) {
          return absl::InternalError(absl::StrFormat(
              "View hash is wrong, %016" PRIx64 " vs %016" PRIx64, view1.hash(),
              h1_rot));
        }

        if (h1_rot != h2) {
          SymmState s1 = game1.calcSymmetryState();

          return absl::InternalError(absl::StrFormat(
              "Hashes are inequal, %016" PRIx64 " vs %016" PRIx64
              "\nGroup op: %s, color invert: %s, symm class: %d",
              h1_rot, h2, view1.op<Group>().toString(),
              view1.areColorsInverted() ? "true" : "false",
              static_cast<int>(s1.symm_class)));
        }
        return true;
      }
    }

    view1.invertColors();
  }

  return false;
}

static absl::StatusOr<bool> eqUnderSymmetries(onoro::Game<NPawns> game1,
                                              onoro::Game<NPawns> game2) {
  using SymmState = typename onoro::Game<NPawns>::BoardSymmetryState;

  SymmState s = game1.calcSymmetryState();
  SymmetryClassOpApplyAndReturn(s.symm_class, _doEqUnderSymmetries, game1,
                                game2);
}

static PyObject* are_symmetries(PyObject* self, PyObject* args) {
  (void) self;

  Py_buffer game_states_proto_in;
  if (!PyArg_ParseTuple(args, "y*", &game_states_proto_in)) {
    std::cerr << "Failed to parse tuple\n";
    return NULL;
  }

  const std::string games_str(
      static_cast<const char*>(game_states_proto_in.buf),
      game_states_proto_in.len);

  onoro::proto::GameStates states;
  if (!states.ParseFromString(games_str)) {
    std::cerr << "Failed to parse protobuf" << std::endl;
    return NULL;
  }

  if (states.state_size() != 2) {
    std::cerr << "Expected 2 game states, but have " << states.state_size()
              << std::endl;
    return NULL;
  }

  onoro::Game<NPawns> games[2];
  const onoro::Game<NPawns>& a = games[0];
  const onoro::Game<NPawns>& b = games[1];

  for (uint32_t i = 0; i < 2; i++) {
    absl::StatusOr<onoro::Game<NPawns>> game_res =
        onoro::Game<NPawns>::LoadState(states.state()[i]);
    if (!game_res.ok()) {
      std::cerr << game_res.status() << std::endl;
      return NULL;
    }
    games[i] = std::move(*game_res);
  }

  absl::StatusOr<bool> res = eqUnderSymmetries(a, b);
  if (!res.ok()) {
    std::cerr << res.status().message() << std::endl;
    Py_RETURN_NONE;
  }

  return PyBool_FromLong(*res);
}

static PyMethodDef are_symmetries_def[] = {
  { "are_symmetries", are_symmetries, METH_VARARGS,
    "Python interface for are_symmetries." },
  { NULL, NULL, 0, NULL }
};

static struct PyModuleDef test_symmetries_cc_module = {
  PyModuleDef_HEAD_INIT,
  "test_symmetries_cc",
  "Python interface for the are_symmetries function.",
  -1,
  are_symmetries_def,
  NULL,
  NULL,
  NULL,
  NULL,
};

PyMODINIT_FUNC PyInit_test_symmetries_cc(void) {
  return PyModule_Create(&test_symmetries_cc_module);
}
