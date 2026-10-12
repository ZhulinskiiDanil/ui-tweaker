#pragma once

// C++ for a node's place on the screen, from a plain description of the node: no Cocos or Geode in it.
// The mod fills in a NodeDesc from the real node; the tests fill it in by hand
#include "Expr.hpp"
#include "Geometry.hpp"
#include "Words.hpp"

#include <optional>
#include <string>
#include <vector>

namespace codegen
{
  // What the position is measured against
  enum class FrameKind
  {
    // The screen: the parent isn't scaled or turned, so the node's place on the screen is its position plus
    // where the parent is. Expressions use `winSize`
    Screen,
    // The parent's content box. Expressions use `parentSize`
    Parent,
  };

  struct Frame
  {
    FrameKind kind = FrameKind::Screen;
    // winSize for the screen, the parent's content size for the parent
    geom::Size extent;
    // For the screen: where the parent's origin is, which the node's position is measured from
    geom::Vec2 origin;
  };

  enum class LayoutKind
  {
    None,
    // Geode's AnchorLayout: moves only children with AnchorLayoutOptions, to an anchor plus an offset
    Anchor,
    // AxisLayout, RowLayout, ColumnLayout: puts every child in a row or a column
    Axis,
    // SimpleAxisLayout, the same for this
    SimpleAxis,
    // Anything else, from another mod
    Other,
  };

  struct LayoutInfo
  {
    LayoutKind kind = LayoutKind::None;
    // "RowLayout", for the comments
    std::string name;
    // For an AnchorLayout: whether the node has AnchorLayoutOptions (without them it isn't moved)
    bool hasAnchorOptions = false;
    // For an axis layout: the gap between the node and the one before it along the axis, if there is one
    std::optional<float> prevGap;
  };

  struct NodeDesc
  {
    std::string id;
    std::string className;
    std::string parentId;
    std::string parentClass;
    // Among the parent's children castable to className, which one this is (for getChildByType)
    int typeIndex = 0;

    geom::Vec2 position;
    float scaleX = 1;
    float scaleY = 1;
    geom::Vec2 anchor;
    int zOrder = 0;

    Frame frame;
    LayoutInfo layout;

    // Which of the properties to write. Position always
    bool scaleChanged = false;
    bool anchorChanged = false;
    bool zOrderChanged = false;
  };

  struct Position
  {
    expr::Linear x;
    expr::Linear y;
  };

  // The position as expressions in the node's frame: each axis from the closest edge or center
  Position positionExpressions(NodeDesc const& node);

  // The 3x3 anchor of an AnchorLayout for expressions in the parent's frame: "TopRight" for (1, 1)
  std::string anchorName(float kx, float ky);

  // What the parent's layout does to the node's position, for the panel and the code. Empty if nothing
  std::string layoutWarning(NodeDesc const& node, words::Lang lang = words::Lang::English);

  // The condition of the `if` that gets the node: `auto node = this->getChildByIDRecursive("play-button")`, or
  // through its parent's ID and getChildByType. Empty if neither has an ID
  std::string lookup(NodeDesc const& node);

  // A block of C++ for one node, ready to paste into a hook of the layer
  std::string generate(NodeDesc const& node);

  // Many nodes in one block, each with a comment saying which node it is
  std::string generateAll(std::vector<NodeDesc> const& nodes);
} // namespace codegen
