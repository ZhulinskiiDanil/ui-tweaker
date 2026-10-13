#include "../src/core/RowFit.hpp"
#include "Check.hpp"

using namespace rowfit;

int main()
{
  // A row given out of order: left to right, gaps 10 and 10, bottoms on one line
  std::vector<geom::Rect> row{{60, 0, 20, 30}, {0, 0, 20, 10}, {30, 0, 20, 20}};
  auto f = fit(row);
  CHECK(f.row);
  CHECK(f.order == std::vector<size_t>({1, 2, 0}));
  CHECK(near(f.gap, 10) && near(f.gapSpread, 0));
  CHECK(f.cross == Align::Start && near(f.crossSpread, 0));
  CHECK(exact(f));
  CHECK(near(f.bounds.x, 0) && near(f.bounds.y, 0) && near(f.bounds.width, 80) && near(f.bounds.height, 30));

  // Centered across, uneven gaps (5 and 15)
  std::vector<geom::Rect> centered{{0, 10, 20, 10}, {25, 5, 20, 20}, {60, 0, 20, 30}};
  f = fit(centered);
  CHECK(f.cross == Align::Center && near(f.crossSpread, 0));
  CHECK(near(f.gap, 10) && near(f.gapSpread, 5));
  CHECK(!exact(f));

  // A column: top to bottom, right edges lined up
  std::vector<geom::Rect> column{{10, 0, 30, 10}, {0, 40, 40, 10}, {20, 20, 20, 10}};
  f = fit(column);
  CHECK(!f.row);
  CHECK(f.order == std::vector<size_t>({1, 2, 0}));
  CHECK(near(f.gap, 10) && near(f.gapSpread, 0));
  CHECK(f.cross == Align::End && near(f.crossSpread, 0));

  // Overlapping: a negative gap
  f = fit({{0, 0, 20, 10}, {15, 0, 20, 10}});
  CHECK(near(f.gap, -5));

  // Fewer than two: nothing
  CHECK(fit({{0, 0, 1, 1}}).order.empty());
  CHECK(!exact(fit({})));

  CHECK(alignName(Align::End) == "End");
  return finish("rowfit");
}
