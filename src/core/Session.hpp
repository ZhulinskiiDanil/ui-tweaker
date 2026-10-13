#pragma once

// The edits kept between runs of the game: every edited node's description, by the layer its code goes in, written
// to a text file and read back. A line a field, "key value", strings escaped; unknown keys are skipped, so an older
// or newer file still reads
#include "Codegen.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace session
{
  using Groups = std::vector<std::pair<std::string, std::vector<codegen::NodeDesc>>>;

  std::string write(Groups const& groups);

  // Nothing if it isn't a session file
  std::optional<Groups> read(std::string_view text);

  // Whether two descriptions are of the same node: by an ID the layer finds, else by the way down to it
  bool sameNode(codegen::NodeDesc const& a, codegen::NodeDesc const& b);
} // namespace session
