#pragma once

// UI_TWEAKER_SELFTEST=1 at launch: in the main menu the mod edits a few nodes with the same calls the mouse
// and the keys make, checks the code it generates and the undo, and leaves the edit mode on for a screenshot.
// Every line of it in the log starts with "Self test"
#include <string>

namespace selftest
{
  // UI_TWEAKER_SELFTEST's value ("1", "play", "tour"...), empty without it
  std::string const& mode();
  bool requested();
  void start();
} // namespace selftest
