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

  if (selftest::requested())
    selftest::start();
}
