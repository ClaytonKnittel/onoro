#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_format.h>
#include <arpa/inet.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "game_state.pb.h"
#include "onoro.h"

static constexpr uint32_t NPawns = 16;

/*
 * Reads <onoro::proto::GameState proto msg> from game_state_proto_in.
 * Returns <onoro::proto::GameStates proto msg>.
 */
static PyObject* gen_next_moves(PyObject* self, PyObject* args) {
  (void) self;

  Py_buffer game_state_proto_in;
  if (!PyArg_ParseTuple(args, "y*", &game_state_proto_in)) {
    std::cerr << "Failed to parse tuple\n";
    return NULL;
  }

  const std::string game_str(static_cast<const char*>(game_state_proto_in.buf),
                             game_state_proto_in.len);

  onoro::proto::GameState state;
  if (!state.ParseFromString(game_str)) {
    std::cerr << "Failed to parse protobuf" << std::endl;
    return NULL;
  }

  absl::StatusOr<onoro::Game<NPawns>> game_res =
      onoro::Game<NPawns>::LoadState(state);
  if (!game_res.ok()) {
    std::cerr << game_res.status() << std::endl;
    return NULL;
  }
  const onoro::Game<NPawns>& game = *game_res;

  onoro::proto::GameStates states;

  if (!game.inPhase2()) {
    game.forEachMove([&game, &states](onoro::P1Move move) {
      onoro::Game g2(game, move);
      *states.add_state() = g2.SerializeState();
      return true;
    });
  } else {
    game.forEachMoveP2([&game, &states](onoro::P2Move move) {
      onoro::Game g2(game, move);
      *states.add_state() = g2.SerializeState();
      return true;
    });
  }

  std::string serialized_msg;
  if (!states.SerializeToString(&serialized_msg)) {
    std::cerr << "Failed to serialize GameStates object" << std::endl;
    return NULL;
  }

  return PyBytes_FromStringAndSize(serialized_msg.c_str(),
                                   serialized_msg.size());
}

static PyMethodDef gen_next_moves_def[] = {
  { "gen_next_moves", gen_next_moves, METH_VARARGS,
    "Python interface for gen_next_moves." },
  { NULL, NULL, 0, NULL }
};

static struct PyModuleDef test_next_moves_cc_module = {
  PyModuleDef_HEAD_INIT, "test_next_moves_cc",
  "Python interface for the gen_next_moves function.", -1, gen_next_moves_def
};

PyMODINIT_FUNC PyInit_test_next_moves_cc(void) {
  return PyModule_Create(&test_next_moves_cc_module);
}
