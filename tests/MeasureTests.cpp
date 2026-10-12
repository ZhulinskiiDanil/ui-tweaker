#include "../src/core/Measure.hpp"
#include "Check.hpp"

using namespace measure;

int main()
{
  // Side by side, overlapping in height: one horizontal gap, at the middle of the overlap
  auto lines = between({0, 0, 10, 10}, {25, 5, 10, 10});
  CHECK(lines.size() == 1);
  CHECK(lines.size() == 1 && near(lines[0].length, 15) && near(lines[0].from.x, 10) && near(lines[0].to.x, 25) &&
        near(lines[0].from.y, 7.5f));

  // The other way round: the same gap
  lines = between({25, 5, 10, 10}, {0, 0, 10, 10});
  CHECK(lines.size() == 1 && near(lines[0].length, 15));

  // Diagonal: a gap on each axis
  lines = between({0, 0, 10, 10}, {20, 30, 5, 5});
  CHECK(lines.size() == 2);
  CHECK(lines.size() == 2 && near(lines[0].length, 10) && near(lines[1].length, 20));

  // Above: one vertical gap
  lines = between({0, 0, 10, 10}, {2, 14, 4, 4});
  CHECK(lines.size() == 1 && near(lines[0].length, 4) && near(lines[0].from.x, 4));

  // Inside: four lines to the outer box's edges
  lines = between({10, 10, 20, 10}, {0, 0, 100, 50});
  CHECK(lines.size() == 4);
  CHECK(lines.size() == 4 && near(lines[0].length, 10) && near(lines[1].length, 70) && near(lines[2].length, 10) &&
        near(lines[3].length, 30));

  // Touching an edge inside: that line is left out
  lines = between({0, 10, 20, 10}, {0, 0, 100, 50});
  CHECK(lines.size() == 3);

  // Overlapping without one inside the other: nothing to measure
  CHECK(between({0, 0, 10, 10}, {5, 5, 10, 10}).empty());
  // The same box
  CHECK(between({1, 2, 3, 4}, {1, 2, 3, 4}).empty());
  return finish("measure");
}
