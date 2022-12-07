
#include <absl/status/statusor.h>
#include <absl/strings/str_format.h>
#include <arpa/inet.h>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "game.h"
#include "game_state.pb.h"

static constexpr uint32_t NPawns = 16;

static absl::StatusOr<std::string> readFromStdin() {
  uint32_t n_bytes;

  if (read(STDIN_FILENO, &n_bytes, sizeof(n_bytes)) != sizeof(n_bytes)) {
    return absl::InternalError("Failed to read protobuf binary size");
  }

  n_bytes = ntohl(n_bytes);

  char* buf = (char*) malloc(n_bytes);
  if (buf == nullptr) {
    return absl::InternalError(
        absl::StrFormat("Failed to malloc %" PRIu32 " bytes", n_bytes));
  }

  if (read(STDIN_FILENO, buf, n_bytes) != n_bytes) {
    return absl::InternalError(absl::StrFormat(
        "Failed to read %u protobuf bytes from stdin", n_bytes));
  }

  return buf;
}

static absl::Status writeToStdout(const std::string& msg) {
  uint32_t msg_size = htonl(msg.size());

  if (write(STDOUT_FILENO, &msg_size, sizeof(msg_size)) == -1) {
    return absl::InternalError("Failed to write message length to stdout");
  }

  if (write(STDOUT_FILENO, msg.data(), msg.size()) == -1) {
    return absl::InternalError(absl::StrFormat(
        "Failed to write message of size %" PRIu32 " to stdout", msg.size()));
  }

  return absl::OkStatus();
}

/*
 * Reads <uint32 size><onoro::proto::GameState proto msg> from stdin.
 * Writes <uint32 size><onoro::proto::GameStates proto msg> to stdout.
 */
int main() {
  // Read the game state from stdin.
  const auto read_res = readFromStdin();
  if (!read_res.ok()) {
    std::cerr << read_res.status().message() << std::endl;
    return -1;
  }

  const std::string game_str = *read_res;
  onoro::proto::GameState state;
  if (!state.ParseFromString(game_str)) {
    std::cerr << "Failed to parse protobuf" << std::endl;
    return -1;
  }

  absl::StatusOr<onoro::Game<NPawns>> game_res =
      onoro::Game<NPawns>::LoadState(state);
  if (!game_res.ok()) {
    std::cerr << game_res.status() << std::endl;
    return -1;
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
    return -1;
  }

  absl::Status write_status = writeToStdout(serialized_msg);
  if (!write_status.ok()) {
    std::cerr << write_status.message() << std::endl;
    return -1;
  }

  return 0;
}
