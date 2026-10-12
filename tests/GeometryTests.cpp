#include "../src/core/Geometry.hpp"
#include "Check.hpp"

using namespace geom;

namespace
{
  bool near(Vec2 a, Vec2 b, float epsilon = 0.01f)
  {
    return ::near(a.x, b.x, epsilon) && ::near(a.y, b.y, epsilon);
  }

  bool sameQuad(Quad const& a, Quad const& b)
  {
    for (size_t i = 0; i < 4; i++)
      if (!near(a.corners[i], b.corners[i]))
        return false;
    return true;
  }

  Quad quadOf(NodeTransform const& node)
  {
    return transform(nodeToParent(node), {0, 0, node.content.width, node.content.height});
  }

  NodeTransform box(Vec2 position, Vec2 anchor, Size content)
  {
    NodeTransform node;
    node.position = position;
    node.anchor = anchor;
    node.content = content;
    return node;
  }

  void anchoredNode()
  {
    auto node = box({10, 20}, {0.5f, 0.5f}, {100, 50});
    auto toParent = nodeToParent(node);
    // The anchor point is at the position, the corner half the size away
    CHECK(near(toParent.apply({50, 25}), {10, 20}));
    CHECK(near(toParent.apply({0, 0}), {-40, -5}));

    node.scaleX = node.scaleY = 2;
    toParent = nodeToParent(node);
    CHECK(near(toParent.apply({50, 25}), {10, 20}));
    CHECK(near(toParent.apply({0, 0}), {-90, -30}));
  }

  void rotatedNode()
  {
    auto node = box({0, 0}, {0, 0}, {10, 10});
    node.rotationX = node.rotationY = 90;
    // Cocos turns clockwise for a positive angle
    CHECK(near(nodeToParent(node).apply({10, 0}), {0, -10}));
    CHECK(near(nodeToParent(node).apply({0, 10}), {10, 0}));
  }

  void ignoredAnchor()
  {
    // Like a CCLayer or a CCMenu: the position is the bottom left corner
    auto node = box({10, 20}, {0.5f, 0.5f}, {100, 50});
    node.ignoreAnchor = true;
    CHECK(near(nodeToParent(node).apply({0, 0}), {10, 20}));
    // Scaled, it grows from the anchor point, which stays at the middle of where the box was
    node.scaleX = node.scaleY = 2;
    CHECK(near(nodeToParent(node).apply({50, 25}), {60, 45}));
  }

  void flippedNode()
  {
    auto node = box({0, 0}, {0.5f, 0.5f}, {100, 50});
    node.scaleX = -1;
    auto quad = quadOf(node);
    CHECK(near(quad.corners[0], {50, -25}));
    // Winds the other way, still contains its middle
    CHECK(quad.contains({0, 0}));
    CHECK(quad.contains({49, 24}));
    CHECK(!quad.contains({51, 0}));
    auto bounds = quad.bounds();
    CHECK(::near(bounds.x, -50) && ::near(bounds.y, -25) && ::near(bounds.width, 100) && ::near(bounds.height, 50));
  }

  void quadContains()
  {
    auto node = box({100, 100}, {0.5f, 0.5f}, {20, 20});
    node.rotationX = node.rotationY = 45;
    auto quad = quadOf(node);
    CHECK(quad.contains({100, 100}));
    // A corner of the unturned box is outside the turned one
    CHECK(!quad.contains({109.5f, 109.5f}));
    CHECK(quad.contains({100, 113}));
    CHECK(near(quad.center(), {100, 100}));
    // Scaled to nothing, a node can't be hovered
    node.scaleX = 0;
    CHECK(!quadOf(node).contains({100, 100}));
  }

  void inverseAndConcat()
  {
    auto node = box({30, -12}, {0.3f, 0.7f}, {40, 25});
    node.scaleX = 1.5f;
    node.scaleY = -0.75f;
    node.rotationX = node.rotationY = 33;
    auto toParent = nodeToParent(node);
    auto back = toParent.inverse();
    Vec2 point{7, 19};
    CHECK(near(back.apply(toParent.apply(point)), point));
    CHECK(near(concat(toParent, back).apply(point), point));

    // A child at (10, 0) in a parent scaled 2 at (100, 100) with its anchor at the corner
    auto parent = box({100, 100}, {0, 0}, {50, 50});
    parent.scaleX = parent.scaleY = 2;
    auto child = box({10, 0}, {0, 0}, {5, 5});
    auto toWorld = concat(nodeToParent(child), nodeToParent(parent));
    CHECK(near(toWorld.apply({0, 0}), {120, 100}));

    CHECK(!(Affine{0, 0, 0, 0, 1, 1}.invertible()));
    CHECK((Affine{1, 0, 0, 1, 5, 7}.isTranslation()));
    CHECK(!(Affine{2, 0, 0, 1, 0, 0}.isTranslation()));
  }

  void deltas()
  {
    // A parent scaled 2: the mouse going 10 pixels is 5 in the parent
    auto parent = box({0, 0}, {0, 0}, {100, 100});
    parent.scaleX = parent.scaleY = 2;
    CHECK(near(localDelta(nodeToParent(parent), {10, 0}), {5, 0}));

    // Turned clockwise by 90: down on the screen is along x in the parent
    parent.scaleX = parent.scaleY = 1;
    parent.rotationX = parent.rotationY = 90;
    CHECK(near(localDelta(nodeToParent(parent), {0, -10}), {10, 0}));

    // Flipped: right on the screen is left in the parent
    parent.rotationX = parent.rotationY = 0;
    parent.scaleX = -1;
    CHECK(near(localDelta(nodeToParent(parent), {10, 3}), {-10, 3}));

    // Moving the position by the delta moves the node by the world delta
    auto child = box({20, 30}, {0.5f, 0.5f}, {10, 10});
    auto toWorld = nodeToParent(parent);
    auto before = toWorld.apply(child.position);
    child.position = child.position + localDelta(toWorld, {7, -4});
    CHECK(near(toWorld.apply(child.position), before + Vec2{7, -4}));
  }

  void resizing()
  {
    // The right edge dragged 10 to the right: 10 wider, the left edge where it was
    auto node = box({100, 100}, {0.5f, 0.5f}, {40, 20});
    auto result = resize(node, {1, 0}, {10, 0});
    CHECK(::near(result.content.width, 50) && ::near(result.content.height, 20));
    auto resized = node;
    resized.position = result.position;
    resized.content = result.content;
    CHECK(near(quadOf(resized).corners[0], quadOf(node).corners[0]));
    CHECK(::near(quadOf(resized).corners[1].x, quadOf(node).corners[1].x + 10));

    // The bottom left corner dragged out, whatever the transform: the top right corner stays
    for (bool ignore : {false, true})
    {
      auto turned = box({33, -7}, {0.2f, 0.9f}, {40, 20});
      turned.scaleX = 1.3f;
      turned.scaleY = -0.6f;
      turned.rotationX = turned.rotationY = 27;
      turned.ignoreAnchor = ignore;
      auto out = resize(turned, {-1, -1}, {-6, -4});
      CHECK(::near(out.content.width, 46) && ::near(out.content.height, 24));
      auto moved = turned;
      moved.position = out.position;
      moved.content = out.content;
      CHECK(near(quadOf(moved).corners[2], quadOf(turned).corners[2]));
    }

    // Never below 0
    auto small = resize(node, {1, 1}, {-100, -100});
    CHECK(::near(small.content.width, 0) && ::near(small.content.height, 0));
    // An edge handle leaves the other axis alone
    auto edge = resize(node, {0, 1}, {50, 5});
    CHECK(::near(edge.content.width, 40) && ::near(edge.content.height, 25));
  }

  void anchors()
  {
    auto node = box({100, 100}, {0.5f, 0.5f}, {40, 20});
    node.scaleX = node.scaleY = 2;
    CHECK(near(positionForAnchor(node, {0, 0}), {60, 80}));

    // Whatever the transform, the node stays where it is
    for (bool ignore : {false, true})
    {
      for (float skew : {0.f, 15.f})
      {
        auto turned = box({33, -7}, {0.2f, 0.9f}, {40, 20});
        turned.scaleX = 1.3f;
        turned.scaleY = -0.6f;
        turned.rotationX = turned.rotationY = 27;
        turned.skewX = skew;
        turned.ignoreAnchor = ignore;
        auto before = quadOf(turned);
        turned.position = positionForAnchor(turned, {1, 0});
        turned.anchor = {1, 0};
        CHECK(sameQuad(before, quadOf(turned)));
      }
    }
  }
} // namespace

int main()
{
  anchoredNode();
  rotatedNode();
  ignoredAnchor();
  flippedNode();
  quadContains();
  inverseAndConcat();
  deltas();
  anchors();
  resizing();
  return finish("geometry");
}
