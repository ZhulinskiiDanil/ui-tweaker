#include "Aspect.hpp"

#include "Expr.hpp"

std::vector<aspect::Shape> const& aspect::shapes()
{
  static std::vector<Shape> const all = {
      {"16:9", 16.f / 9.f},
      {"16:10", 16.f / 10.f},
      {"4:3", 4.f / 3.f},
      {"21:9", 21.f / 9.f},
  };
  return all;
}

float aspect::widthFor(Shape const& shape, float height)
{
  return height * shape.ratio;
}

geom::Rect aspect::move(geom::Rect const& box, float width, float newWidth)
{
  // The box's center keeps its distance to the closest of the three
  auto center = expr::nearest(box.midX(), width);
  float midX = expr::evaluate(center, newWidth);
  return {midX - box.width / 2, box.y, box.width, box.height};
}

bool aspect::fits(geom::Rect const& box, float width, float height)
{
  constexpr float SLACK = 0.5f;
  return box.minX() >= -SLACK && box.maxX() <= width + SLACK && box.minY() >= -SLACK && box.maxY() <= height + SLACK;
}

std::string aspect::offScreenOn(geom::Rect const& box, geom::Size screen)
{
  std::string names;
  for (auto const& shape : shapes())
  {
    float width = widthFor(shape, screen.height);
    if (fits(move(box, screen.width, width), width, screen.height))
      continue;
    names += (names.empty() ? "" : ", ") + shape.name;
  }
  return names;
}
