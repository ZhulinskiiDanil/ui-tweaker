#include "../src/core/Aspect.hpp"
#include "Check.hpp"

using namespace aspect;

int main()
{
  CHECK(shapes().size() == 4);
  CHECK(near(widthFor(shapes()[0], 320), 568.89f));
  CHECK(near(widthFor(shapes()[2], 320), 426.67f));

  // 16:9 at 569 x 320. A box by the right edge stays by it on 4:3
  geom::Rect right{569 - 30, 100, 20, 20};
  auto moved = move(right, 569, 426.67f);
  CHECK(near(moved.maxX(), 426.67f - 10));
  CHECK(near(moved.y, 100) && near(moved.width, 20));

  // A box 250 right of the middle fits on 16:9
  geom::Rect far{284.5f + 250 - 10, 100, 20, 20};
  CHECK(fits(far, 569, 320));
  // Its center is 250 from the middle and 34.5 from the right edge: the right edge is closer, so it stays by it
  CHECK(offScreenOn(far, {569, 320}).empty());

  // 120 right of the middle, closer to it than to the right edge, and 200 wide: off the screen on 4:3 only
  geom::Rect wide{284.5f + 120 - 100, 100, 200, 20};
  CHECK(offScreenOn(wide, {569, 320}) == "4:3");

  // Wider than the narrowest screen: off on every narrower one
  geom::Rect huge{284.5f - 260, 0, 520, 20};
  CHECK(offScreenOn(huge, {569, 320}) == "16:10, 4:3");

  // Off the screen already: off on all of them
  geom::Rect below{100, -50, 20, 20};
  CHECK(offScreenOn(below, {569, 320}) == "16:9, 16:10, 4:3, 21:9");

  // In the middle: fits everywhere
  CHECK(offScreenOn({274.5f, 150, 20, 20}, {569, 320}).empty());
  CHECK(fits({0, 0, 569, 320}, 569, 320));
  return finish("aspect");
}
