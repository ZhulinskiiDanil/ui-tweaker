#include "../src/core/Snap.hpp"
#include "Check.hpp"

using namespace snap;

namespace
{
  std::vector<Guide> screen()
  {
    std::vector<Guide> guides;
    addBox(guides, {0, 0, 569, 320});
    return guides;
  }

  void toTheScreen()
  {
    // A 40 x 20 box whose center is 2 right of the screen's middle: it goes to the middle
    geom::Rect box{284.5f + 2 - 20, 100, 40, 20};
    auto result = snap::snap(box, screen(), 3);
    CHECK(near(result.correction.x, -2));
    CHECK(near(result.correction.y, 0));
    CHECK(result.lines.size() == 1);
    CHECK(result.lines.size() == 1 && result.lines[0].vertical && near(result.lines[0].at, 284.5f));
    // The line goes the whole height of the screen
    CHECK(result.lines.size() == 1 && near(result.lines[0].from, 0) && near(result.lines[0].to, 320));

    // Too far: nothing
    box.x += 10;
    result = snap::snap(box, screen(), 3);
    CHECK(near(result.correction.x, 0) && result.lines.empty());
  }

  void edges()
  {
    // The right edge 1.5 from the screen's: snaps on x, the top 2 below the top: snaps on y too
    geom::Rect box{569 - 40 - 1.5f, 320 - 20 - 2, 40, 20};
    auto result = snap::snap(box, screen(), 3);
    CHECK(near(result.correction.x, 1.5f));
    CHECK(near(result.correction.y, 2));
    CHECK(result.lines.size() == 2);
  }

  void closest()
  {
    std::vector<Guide> guides;
    guides.push_back({true, 100, 0, 10});
    guides.push_back({true, 103, 50, 60});
    // The left edge at 102: 103 is closer
    auto result = snap::snap({102, 20, 10, 10}, guides, 5);
    CHECK(near(result.correction.x, 1));
    CHECK(result.lines.size() == 1 && near(result.lines[0].at, 103));
    // The line reaches from the guide's span to the box
    CHECK(result.lines.size() == 1 && near(result.lines[0].from, 20) && near(result.lines[0].to, 60));
  }

  void neighbours()
  {
    // Lined up with a sibling's top and its center at once: one correction, two lines
    std::vector<Guide> guides;
    addBox(guides, {200, 200, 50, 30});
    // The box's top at 229 (sibling's top 230), its left at 199 (sibling's left 200)
    auto result = snap::snap({199, 209, 20, 20}, guides, 3);
    CHECK(near(result.correction.x, 1) && near(result.correction.y, 1));
    CHECK(result.lines.size() == 2);

    // Two guides at the same place are one line
    guides.clear();
    guides.push_back({false, 50, 0, 10});
    guides.push_back({false, 50, 100, 120});
    result = snap::snap({40, 49, 5, 5}, guides, 3);
    CHECK(result.lines.size() == 1 && near(result.lines[0].from, 0) && near(result.lines[0].to, 120));
  }

  void nothing()
  {
    auto result = snap::snap({10, 10, 5, 5}, {}, 3);
    CHECK(near(result.correction.x, 0) && near(result.correction.y, 0) && result.lines.empty());
  }
} // namespace

int main()
{
  toTheScreen();
  edges();
  closest();
  neighbours();
  nothing();
  return finish("snap");
}
