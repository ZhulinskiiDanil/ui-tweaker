#include "Editor.hpp"
#include "SelfTest.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// ! --- Start --- !

$on_mod(Loaded)
{
  // The only thing listening while the edit mode is off: Geode's own keybind, no hooks
  listenForKeybindSettingPresses("toggle-key", [](Keybind const&, bool down, bool repeat, double)
                                 {
                                   if (down && !repeat)
                                     editor::toggle(); });

  // The edits kept from the last time, for "Copy all" (the self test starts without them, in a file of its own)
  if (!selftest::requested())
    editor::loadSession();

  if (selftest::requested())
    selftest::start();
}

$on_game(Exiting)
{
  editor::saveSessionNow();
}
