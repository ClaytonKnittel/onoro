#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <absl/status/statusor.h>

#include <cstdio>

#include "game_state.pb.h"
#include "onoro.h"

static constexpr uint32_t NPawns = 16;

template <class SymmetryClassOp>
static bool _doEqUnderSymmetries(onoro::Game<NPawns> game1,
                                 onoro::Game<NPawns> game2) {
  typedef typename SymmetryClassOp::Group Group;
  onoro::GameEq<NPawns> games_eq;

  onoro::GameView<NPawns> view1(&game1);

  for (bool swap_colors : { false, true }) {
    (void) swap_colors;

    for (uint32_t op_ord = 0; op_ord < Group::order(); op_ord++) {
      Group op(op_ord);
      view1.setOp(op);

      if (games_eq(view1, game2)) {
        return true;
      }
    }

    view1.invertColors();
  }

  return false;
}

static bool eqUnderSymmetries(onoro::Game<NPawns> game1,
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

  return PyBool_FromLong(eqUnderSymmetries(a, b));
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