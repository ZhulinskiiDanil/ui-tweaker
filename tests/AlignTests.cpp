#include "../src/core/Align.hpp"
#include "Check.hpp"

using namespace align;

int main()
{
  std::vector<geom::Rect> boxes{{0, 0, 10, 10}, {30, 20, 20, 5}, {12, -8, 4, 4}};
  auto all = bounds(boxes);
  CHECK(near(all.x, 0) && near(all.y, -8) && near(all.width, 50) && near(all.height, 33));
  CHECK(bounds({}).width == 0);

  // To the box of them all
  auto moves = to(boxes, Edge::Left, all);
  CHECK(moves.size() == 3 && near(moves[0].x, 0) && near(moves[1].x, -30) && near(moves[2].x, -12));
  CHECK(near(moves[1].y, 0));
  moves = to(boxes, Edge::Right, all);
  CHECK(near(moves[0].x, 40) && near(moves[1].x, 0) && near(moves[2].x, 34));
  moves = to(boxes, Edge::CenterX, all);
  CHECK(near(moves[0].x, 20) && near(moves[1].x, -15) && near(moves[2].x, 11));
  moves = to(boxes, Edge::Top, all);
  CHECK(near(moves[0].y, 15) && near(moves[1].y, 0) && near(moves[2].y, 29) && near(moves[0].x, 0));
  moves = to(boxes, Edge::Bottom, all);
  CHECK(near(moves[0].y, -8) && near(moves[1].y, -28) && near(moves[2].y, 0));
  moves = to(boxes, Edge::CenterY, all);
  CHECK(near(moves[0].y, 3.5f) && near(moves[1].y, -14) && near(moves[2].y, 14.5f));

  // One box in its parent: centered in it
  moves = to({{10, 10, 20, 10}}, Edge::CenterX, {0, 0, 100, 50});
  CHECK(moves.size() == 1 && near(moves[0].x, 30));

  // Distributed: the first and the last stay, the gaps are the same
  std::vector<geom::Rect> row{{0, 0, 10, 10}, {90, 0, 30, 10}, {15, 0, 20, 10}};
  moves = distribute(row, true);
  CHECK(moves.size() == 3);
  CHECK(near(moves[0].x, 0) && near(moves[1].x, 0));
  // Gap: (120 - 0 - 60) / 2 = 30, so the middle one starts at 10 + 30 = 40
  CHECK(near(moves[2].x, 25) && near(moves[2].y, 0));

  // Vertically
  std::vector<geom::Rect> column{{0, 0, 10, 10}, {0, 12, 10, 10}, {0, 100, 10, 10}};
  moves = distribute(column, false);
  CHECK(near(moves[0].y, 0) && near(moves[2].y, 0) && near(moves[1].y, 38) && near(moves[1].x, 0));

  // Overlapping boxes: a negative gap, still the same between each two
  std::vector<geom::Rect> tight{{0, 0, 20, 10}, {5, 0, 20, 10}, {30, 0, 20, 10}, {20, 0, 20, 10}};
  moves = distribute(tight, true);
  // Order by middles: 0 (10), 1 (15), 3 (30), 2 (40); gap (50 - 80) / 3 = -10
  CHECK(near(moves[0].x, 0) && near(moves[1].x, 5) && near(moves[3].x, 0) && near(moves[2].x, 0));
  float starts[4];
  for (int i = 0; i < 4; i++)
    starts[i] = tight[i].x + moves[i].x;
  CHECK(near(starts[1] - (starts[0] + 20), -10) && near(starts[3] - (starts[1] + 20), -10) &&
        near(starts[2] - (starts[3] + 20), -10));

  // Fewer than three: nothing moves
  moves = distribute({{0, 0, 1, 1}, {9, 0, 1, 1}}, true);
  CHECK(moves.size() == 2 && near(moves[0].x, 0) && near(moves[1].x, 0));

  return finish("align");
}
