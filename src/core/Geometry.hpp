#pragma once

// Points, rects and the transforms between a node's space and its parent's, without Cocos or Geode in it.
// The transform is the one CCNode::nodeToParentTransform builds, so the tests can check the math by hand
#include <array>

namespace geom
{
  struct Vec2
  {
    float x = 0;
    float y = 0;

    Vec2 operator+(Vec2 other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(Vec2 other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float factor) const { return {x * factor, y * factor}; }
    bool operator==(Vec2 const&) const = default;
  };

  struct Size
  {
    float width = 0;
    float height = 0;

    bool operator==(Size const&) const = default;
  };

  // Axis-aligned, from its bottom left corner
  struct Rect
  {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    float minX() const { return x; }
    float midX() const { return x + width / 2; }
    float maxX() const { return x + width; }
    float minY() const { return y; }
    float midY() const { return y + height / 2; }
    float maxY() const { return y + height; }
    bool contains(Vec2 point) const;
  };

  // x' = a*x + c*y + tx, y' = b*x + d*y + ty: Cocos' CCAffineTransform
  struct Affine
  {
    float a = 1;
    float b = 0;
    float c = 0;
    float d = 1;
    float tx = 0;
    float ty = 0;

    Vec2 apply(Vec2 point) const;
    // Without the translation: for deltas and directions
    Vec2 applyLinear(Vec2 vector) const;
    float determinant() const { return a * d - b * c; }
    // Identity if it can't be inverted (a node scaled to 0)
    Affine inverse() const;
    bool invertible() const;
    // Translation only, no scale, rotation or skew
    bool isTranslation(float epsilon = 1e-4f) const;
  };

  // `first`, then `second`: CCAffineTransformConcat(first, second)
  Affine concat(Affine const& first, Affine const& second);

  // What CCNode keeps for its transform
  struct NodeTransform
  {
    Vec2 position;
    Vec2 anchor;
    Size content;
    float scaleX = 1;
    float scaleY = 1;
    float rotationX = 0;
    float rotationY = 0;
    float skewX = 0;
    float skewY = 0;
    bool ignoreAnchor = false;
  };

  // CCNode::nodeToParentTransform, step by step
  Affine nodeToParent(NodeTransform const& node);

  // A rect after a transform: four corners, in order bottom left, bottom right, top right, top left
  struct Quad
  {
    std::array<Vec2, 4> corners;

    Rect bounds() const;
    // Inside or on the edge, for any winding (a flipped node's quad winds the other way)
    bool contains(Vec2 point) const;
    Vec2 center() const;
  };

  Quad transform(Affine const& affine, Rect const& rect);

  // The change of a node's position (in its parent's space) that moves it by `worldDelta` on the screen.
  // `parentToWorld` is the parent's node-to-world transform; a rotated, scaled or flipped parent turns the delta
  Vec2 localDelta(Affine const& parentToWorld, Vec2 worldDelta);

  // The position that keeps the node where it is on the screen after its anchor point changes to `anchor`
  Vec2 positionForAnchor(NodeTransform const& node, Vec2 anchor);

  struct Resized
  {
    Vec2 position;
    Size content;
  };

  // A resize handle dragged: `handle` is which side, -1 (left, bottom), 0 (that axis isn't resized) or 1 (right,
  // top) on each axis; `delta` is how far, in the node's own space (before its scale). The opposite side stays
  // where it is in the parent, whatever the anchor point, scale or rotation; the size doesn't go below 0
  Resized resize(NodeTransform const& node, Vec2 handle, Vec2 delta);
} // namespace geom
