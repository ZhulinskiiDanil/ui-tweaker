#include "Geometry.hpp"

#include <algorithm>
#include <cmath>

namespace
{
  constexpr float PI = 3.14159265358979f;

  float radians(float degrees) { return degrees * PI / 180.f; }

  float cross(geom::Vec2 origin, geom::Vec2 a, geom::Vec2 b)
  {
    return (a.x - origin.x) * (b.y - origin.y) - (a.y - origin.y) * (b.x - origin.x);
  }
} // namespace

bool geom::Rect::contains(Vec2 point) const
{
  return point.x >= minX() && point.x <= maxX() && point.y >= minY() && point.y <= maxY();
}

geom::Vec2 geom::Affine::apply(Vec2 point) const
{
  return {a * point.x + c * point.y + tx, b * point.x + d * point.y + ty};
}

geom::Vec2 geom::Affine::applyLinear(Vec2 vector) const
{
  return {a * vector.x + c * vector.y, b * vector.x + d * vector.y};
}

bool geom::Affine::invertible() const
{
  return std::fabs(determinant()) > 1e-12f;
}

geom::Affine geom::Affine::inverse() const
{
  if (!invertible())
    return {};
  float det = 1.f / determinant();
  return {
      det * d,
      -det * b,
      -det * c,
      det * a,
      det * (c * ty - d * tx),
      det * (b * tx - a * ty),
  };
}

bool geom::Affine::isTranslation(float epsilon) const
{
  return std::fabs(a - 1) < epsilon && std::fabs(d - 1) < epsilon && std::fabs(b) < epsilon && std::fabs(c) < epsilon;
}

geom::Affine geom::concat(Affine const& first, Affine const& second)
{
  return {
      first.a * second.a + first.b * second.c,
      first.a * second.b + first.b * second.d,
      first.c * second.a + first.d * second.c,
      first.c * second.b + first.d * second.d,
      first.tx * second.a + first.ty * second.c + second.tx,
      first.tx * second.b + first.ty * second.d + second.ty,
  };
}

geom::Affine geom::nodeToParent(NodeTransform const& node)
{
  Vec2 anchorInPoints{node.anchor.x * node.content.width, node.anchor.y * node.content.height};
  float x = node.position.x;
  float y = node.position.y;
  if (node.ignoreAnchor)
  {
    x += anchorInPoints.x;
    y += anchorInPoints.y;
  }

  float cx = 1, sx = 0, cy = 1, sy = 0;
  if (node.rotationX != 0 || node.rotationY != 0)
  {
    float radiansX = -radians(node.rotationX);
    float radiansY = -radians(node.rotationY);
    cx = std::cos(radiansX);
    sx = std::sin(radiansX);
    cy = std::cos(radiansY);
    sy = std::sin(radiansY);
  }

  bool skewed = node.skewX != 0 || node.skewY != 0;
  bool anchored = anchorInPoints.x != 0 || anchorInPoints.y != 0;
  if (!skewed && anchored)
  {
    x += cy * -anchorInPoints.x * node.scaleX + -sx * -anchorInPoints.y * node.scaleY;
    y += sy * -anchorInPoints.x * node.scaleX + cx * -anchorInPoints.y * node.scaleY;
  }

  Affine result{cy * node.scaleX, sy * node.scaleX, -sx * node.scaleY, cx * node.scaleY, x, y};
  if (skewed)
  {
    Affine skew{1, std::tan(radians(node.skewY)), std::tan(radians(node.skewX)), 1, 0, 0};
    result = concat(skew, result);
    if (anchored)
    {
      // CCAffineTransformTranslate
      result.tx += result.a * -anchorInPoints.x + result.c * -anchorInPoints.y;
      result.ty += result.b * -anchorInPoints.x + result.d * -anchorInPoints.y;
    }
  }
  return result;
}

geom::Rect geom::Quad::bounds() const
{
  float minX = corners[0].x, maxX = corners[0].x, minY = corners[0].y, maxY = corners[0].y;
  for (auto const& corner : corners)
  {
    minX = std::min(minX, corner.x);
    maxX = std::max(maxX, corner.x);
    minY = std::min(minY, corner.y);
    maxY = std::max(maxY, corner.y);
  }
  return {minX, minY, maxX - minX, maxY - minY};
}

bool geom::Quad::contains(Vec2 point) const
{
  // Inside a convex quad: on the same side of every edge
  bool positive = false, negative = false;
  for (size_t i = 0; i < corners.size(); i++)
  {
    float side = cross(corners[i], corners[(i + 1) % corners.size()], point);
    if (side > 1e-6f)
      positive = true;
    else if (side < -1e-6f)
      negative = true;
  }
  if (positive && negative)
    return false;
  // A quad squashed to a line or a point contains nothing
  return positive || negative;
}

geom::Vec2 geom::Quad::center() const
{
  return (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;
}

geom::Quad geom::transform(Affine const& affine, Rect const& rect)
{
  return {{
      affine.apply({rect.minX(), rect.minY()}),
      affine.apply({rect.maxX(), rect.minY()}),
      affine.apply({rect.maxX(), rect.maxY()}),
      affine.apply({rect.minX(), rect.maxY()}),
  }};
}

geom::Vec2 geom::localDelta(Affine const& parentToWorld, Vec2 worldDelta)
{
  return parentToWorld.inverse().applyLinear(worldDelta);
}

geom::Vec2 geom::positionForAnchor(NodeTransform const& node, Vec2 anchor)
{
  // The anchor doesn't change the linear part of the transform, only its translation, which is the position
  // plus something that depends on the anchor alone. Keeping the translation keeps the node in place
  auto offset = [&](Vec2 at)
  {
    auto copy = node;
    copy.position = {};
    copy.anchor = at;
    auto toParent = nodeToParent(copy);
    return Vec2{toParent.tx, toParent.ty};
  };
  return node.position + offset(node.anchor) - offset(anchor);
}
