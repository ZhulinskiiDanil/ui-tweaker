#pragma once

// With Improved Logger installed: the selected node and the count of edited nodes pinned in its console, a
// marker when code is copied (the code itself is in the log, which its console shows). Without it, nothing
#include <string>

namespace logger_link
{
  // Pins the selected node's line and how many nodes are edited, when they change
  void pin(std::string const& selected, size_t edited);
  // A marker line in its console
  void mark(std::string const& label);
  // Takes the pinned values away: the edit mode is off
  void clear();
} // namespace logger_link
