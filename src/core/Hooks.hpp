#pragma once

// The hooks the code goes into: for the layers mods hook most, the $modify that makes them, with the signature
// of the function their interface is made in (from Geode's bindings), so "Copy all" can be pasted as it is
#include <optional>
#include <string>
#include <string_view>

namespace hooks
{
  struct Hook
  {
    std::string className;
    // "bool init(int page)"
    std::string signature;
    // "LevelSelectLayer::init(page)"
    std::string original;
    // init returns whether it worked; customSetup returns nothing
    bool returnsBool = true;
  };

  std::optional<Hook> find(std::string_view className);

  // The code as the body of the class's hook, after the original made the interface. Unknown classes get the code
  // with a comment saying where it goes
  std::string wrap(std::string_view className, std::string const& code);
} // namespace hooks
