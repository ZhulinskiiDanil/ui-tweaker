#include "../src/core/Expr.hpp"
#include "Check.hpp"

#include <string>

using namespace expr;

namespace
{
  std::optional<float> screen(std::string_view name)
  {
    if (name == "winSize.width")
      return 569.f;
    if (name == "winSize.height")
      return 320.f;
    return std::nullopt;
  }

  void nearestEdge()
  {
    // 16:9 at GD's height: 569 x 320
    CHECK((nearest(324.5f, 569) == Linear{0.5f, 40}));
    CHECK((nearest(544, 569) == Linear{1, -25}));
    CHECK((nearest(25, 569) == Linear{0, 25}));
    CHECK((nearest(295, 320) == Linear{1, -25}));
    // A tie between the start and the center goes to the center, then to the start before the end
    CHECK((nearest(25, 100) == Linear{0.5f, -25}));
    CHECK((nearest(50, 100) == Linear{0.5f, 0}));
    // Outside the extent: still the closest
    CHECK((nearest(-30, 100) == Linear{0, -30}));
    CHECK((nearest(130, 100) == Linear{1, 30}));
    // Nothing to measure from
    CHECK((nearest(42, 0) == Linear{0, 42}));
    CHECK((nearest(42, -5) == Linear{0, 42}));
  }

  void numbers()
  {
    CHECK(number(40) == "40.f");
    CHECK(number(12.5f) == "12.5f");
    CHECK(number(-3.25f) == "-3.25f");
    CHECK(number(0) == "0.f");
    CHECK(number(-0.001f) == "0.f");
    CHECK(number(1.005f) == "1.f" || number(1.005f) == "1.01f");
    CHECK(number(0.333333f) == "0.33f");
    CHECK(number(1234.567f) == "1234.57f");
  }

  void formats()
  {
    CHECK(format({0.5f, 40}, "winSize.width") == "winSize.width / 2 + 40.f");
    CHECK(format({1, -25}, "winSize.height") == "winSize.height - 25.f");
    CHECK(format({0, 25}, "winSize.width") == "25.f");
    CHECK(format({0.5f, 0}, "winSize.width") == "winSize.width / 2");
    CHECK(format({1, 0.001f}, "parentSize.width") == "parentSize.width");
    CHECK(format({-0.5f, 10}, "winSize.width") == "-winSize.width / 2 + 10.f");
    CHECK(format({-1, -3}, "winSize.width") == "-winSize.width - 3.f");
    CHECK(format({0.5f, -12.75f}, "winSize.width") == "winSize.width / 2 - 12.75f");
  }

  void relative()
  {
    // The right edge, seen from a parent whose origin is at the middle of the screen
    CHECK((relativeTo({1, -25}, {0.5f, 0}) == Linear{0.5f, -25}));
    CHECK((relativeTo({0.5f, 40}, {0.5f, 0}) == Linear{0, 40}));
    CHECK((relativeTo({0, 25}, {0, 100}) == Linear{0, -75}));
    CHECK(near(evaluate(Linear{0.5f, -25}, 569), 259.5f));
  }

  void evaluating()
  {
    CHECK(near(*evaluate("winSize.width / 2 + 40.f", screen), 324.5f));
    CHECK(near(*evaluate("winSize.height - 25.f", screen), 295));
    CHECK(near(*evaluate("-winSize.width / 2 + 10.f", screen), -274.5f));
    CHECK(near(*evaluate("12.5f", screen), 12.5f));
    CHECK(near(*evaluate("-3.25f", screen), -3.25f));
    CHECK(near(*evaluate("(1 + 2) * 3", screen), 9));
    CHECK(near(*evaluate("2 * -3", screen), -6));
    CHECK(near(*evaluate("winSize.width * 0.25f", screen), 142.25f));
    CHECK(near(*evaluate(".5f", screen), 0.5f));
    CHECK(!evaluate("unknown.width", screen));
    CHECK(!evaluate("1 +", screen));
    CHECK(!evaluate("(1", screen));
    CHECK(!evaluate("1 / 0", screen));
    CHECK(!evaluate("", screen));
    CHECK(!evaluate("4 4", screen));
    CHECK(!evaluate(".", screen));

    // Every format reads back as what it was made from
    for (float k : {0.f, 0.5f, 1.f, -0.5f, -1.f})
      for (float offset : {0.f, 40.f, -25.f, 12.5f, -0.75f})
        CHECK(near(*evaluate(format({k, offset}, "winSize.width"), screen), evaluate(Linear{k, offset}, 569)));
  }

  void calls()
  {
    auto args = callArguments("  node->setPosition({winSize.width / 2 + 40.f, winSize.height - 25.f});", "setPosition");
    CHECK(args.size() == 2);
    CHECK(args.size() == 2 && args[0] == "winSize.width / 2 + 40.f");
    CHECK(args.size() == 2 && args[1] == "winSize.height - 25.f");

    args = callArguments("node->setScale(1.2f);", "setScale");
    CHECK(args.size() == 1 && args[0] == "1.2f");

    // Parentheses inside an argument
    args = callArguments("f({(a + b) * 2, c})", "f");
    CHECK(args.size() == 2 && args[0] == "(a + b) * 2" && args[1] == "c");

    args = callArguments("->setOffset({-25.f, -10.f}));", "setOffset");
    CHECK(args.size() == 2 && args[0] == "-25.f" && args[1] == "-10.f");

    CHECK(callArguments("node->setScale(1.2f);", "setPosition").empty());
    CHECK(callArguments("node->setPosition({1, 2", "setPosition").empty());
    // setScaleX isn't setScale
    args = callArguments("node->setScaleX(2.f); node->setScale(3.f);", "setScale");
    CHECK(args.size() == 1 && args[0] == "3.f");
  }
} // namespace

int main()
{
  nearestEdge();
  numbers();
  formats();
  relative();
  evaluating();
  calls();
  return finish("expr");
}
