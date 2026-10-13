#include "../src/core/Grid.hpp"
#include "Check.hpp"

#include <algorithm>

int main()
{
  // Offsets rounded to the step, the anchor kept
  auto r = grid::round({0.5f, 37.4f}, 5);
  CHECK(near(r.k, 0.5f) && near(r.offset, 35));
  r = grid::round({1, -23}, 5);
  CHECK(near(r.k, 1) && near(r.offset, -25));
  r = grid::round({0, 12.6f}, 0);
  CHECK(near(r.offset, 12.6f));

  // The lines of a 569 wide screen every 10: from 0 up to a quarter, from the middle both ways, from the end back
  auto lines = grid::lines(569, 10);
  auto has = [&](float value)
  { return std::any_of(lines.begin(), lines.end(), [&](float line) { return near(line, value); }); };
  CHECK(has(0) && has(10) && has(140) && has(284.5f) && has(274.5f) && has(294.5f) && has(569) && has(559));
  // Past the quarter (142.25) values are the middle's: 284.5 - 140 = 144.5, not 150
  CHECK(!has(150) && has(144.5f));
  // Each line once, in order
  CHECK(std::is_sorted(lines.begin(), lines.end()));
  CHECK(grid::lines(100, 0).empty() && grid::lines(0, 5).empty());
  return finish("grid");
}
