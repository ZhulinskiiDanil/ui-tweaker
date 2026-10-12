#include "../src/core/Explain.hpp"
#include "Check.hpp"

#include <string>

using namespace explain;

namespace
{
  bool any(std::vector<std::string> const& lines, std::string const& part)
  {
    for (auto const& line : lines)
      if (line.find(part) != std::string::npos)
        return true;
    return false;
  }
} // namespace

int main()
{
  // Nothing odd: nothing to say
  CHECK(reasons({}).empty());

  Facts facts;
  facts.parent = "main-menu (CCMenu)";
  facts.layoutName = "RowLayout";
  facts.layoutMovesIt = true;
  auto lines = reasons(facts);
  CHECK(lines.size() == 1 && any(lines, "main-menu (CCMenu) has a RowLayout"));

  // A layout that doesn't move it (an AnchorLayout without options) isn't a reason
  facts.layoutMovesIt = false;
  CHECK(reasons(facts).empty());

  Facts odd;
  odd.hiddenBy = "bottom-menu (CCMenu)";
  odd.transparent = true;
  odd.zeroSize = true;
  odd.zeroScale = true;
  odd.ignoresAnchor = true;
  odd.runningActions = 2;
  odd.clippedBy = "CCClippingNode";
  odd.parentTurned = true;
  odd.outsideScreen = true;
  lines = reasons(odd);
  CHECK(lines.size() == 9);
  CHECK(any(lines, "bottom-menu (CCMenu) is invisible"));
  CHECK(any(lines, "opacity is 0"));
  CHECK(any(lines, "scale is 0"));
  CHECK(any(lines, "content size is 0 x 0"));
  CHECK(any(lines, "ignores its anchor point"));
  CHECK(any(lines, "2 actions are running"));
  CHECK(any(lines, "Outside CCClippingNode"));
  CHECK(any(lines, "parent is turned"));
  CHECK(any(lines, "Off the screen"));

  odd = {};
  odd.runningActions = 1;
  CHECK(any(reasons(odd), "1 action is running"));
  return finish("explain");
}
