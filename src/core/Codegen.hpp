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
    // For an AnchorLayout: the anchor the node has now, as the k of each axis (0, 0.5, 1). The code keeps it, only
    // the offset changes: the anchor is the mod developer's choice
    std::optional<geom::Vec2> anchorNow;
    // For an axis layout: the gap between the node and the one before it along the axis, if there is one
    std::optional<float> prevGap;
  };

  struct NodeDesc
  {
    std::string id;
    std::string className;
    std::string parentId;
    std::string parentClass;
    // The layer whose hook the code goes in: the closest of the node's parents that's a layer mods hook, or the
    // node's top one in the scene
    std::string layer;
    // Among the parent's children castable to className, which one this is (for getChildByType)
    int typeIndex = 0;
    // A class the code can't name (another mod's, in a namespace): typeIndex is which child it is of all, for
    // getChildByIndex
    bool byIndex = false;

    // A node whose parent has no ID either: the way down to it from its closest parent with an ID (pathRoot), or
    // from the layer whose hook the code goes in (pathFromLayer), one step a level, the node's own the last. Empty:
    // through parentId and the fields above
    struct PathStep
    {
      // An ID none of the siblings before it has: getChildByID
      std::string id;
      std::string className;
      int typeIndex = 0;
      bool byIndex = false;
      // A sprite's or a button's frame that none of the siblings before it has: getChildBySpriteFrameName finds
      // it whatever is added around it
      std::string frameName;
    };
    std::string pathRoot;
    bool pathFromLayer = false;
    std::vector<PathStep> path;
    // Another node in the layer has the same ID and getChildByIDRecursive finds that one first: then the path
    bool idUnique = true;

    geom::Vec2 position;
    float scaleX = 1;
    float scaleY = 1;
    geom::Vec2 anchor;
    int zOrder = 0;
    geom::Size size;
    // The node lays out its own children: a new size needs updateLayout()
    bool hasLayout = false;

    Frame frame;
    LayoutInfo layout;

    // Which of the properties to write. Position always
    bool scaleChanged = false;
    bool anchorChanged = false;
    bool zOrderChanged = false;
    bool sizeChanged = false;
    bool visible = true;
    bool visibleChanged = false;
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

  // The conditions of the nested `if`s that get the node, the last one declaring `node`:
  // `auto node = this->getChildByIDRecursive("play-button")`, or through its parent's ID and getChildByType, or
  // down a path from a parent with an ID or the layer. Empty if there's no way to it
  std::vector<std::string> lookup(NodeDesc const& node);

  // A block of C++ for one node, ready to paste into a hook of the layer
  std::string generate(NodeDesc const& node);

  // Many nodes in one block, each with a comment saying which node it is
  std::string generateAll(std::vector<NodeDesc> const& nodes);

  // The code is made with braces on their own lines and 2 spaces (Allman); this gives it with the opening brace at
  // the end of the line before and 4 spaces (K&R), as most Geode mods are written
  std::string toKAndR(std::string const& code);
} // namespace codegen
