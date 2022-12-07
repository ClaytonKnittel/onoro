
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
  }

  return 0;
}
