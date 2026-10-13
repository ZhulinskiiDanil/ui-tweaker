#pragma once

// The nodes the generated code is compiled and run for: one of every kind of code the generator writes. Each
// with its own IDs, so they can all be in one scene for the code of all of them
#include "../../src/core/Codegen.hpp"

#include <string>
#include <vector>

inline constexpr geom::Size WIN{569, 320};
inline constexpr geom::Size BOX{200, 100};

inline std::vector<codegen::NodeDesc> samples()
{
  using namespace codegen;
  std::vector<NodeDesc> all;
  // add returns a pointer into it: no reallocation
  all.reserve(32);
  auto add = [&](std::string id, std::string className, std::string parentId, geom::Vec2 position, Frame frame)
  {
    NodeDesc node;
    node.id = std::move(id);
    node.className = std::move(className);
    node.parentId = std::move(parentId);
    node.parentClass = "CCMenu";
    node.position = position;
    node.frame = frame;
    all.push_back(node);
    return &all.back();
  };
  Frame screen{FrameKind::Screen, WIN, {0, 0}};
  Frame centered{FrameKind::Screen, WIN, {284.5f, 160}};
  Frame box{FrameKind::Parent, BOX, {}};

  // 0: near the screen's right edge and top
  add("s0", "CCSprite", "p0", {324.5f, 295}, screen);
  // 1: a button in a menu as made, scaled
  auto one = add("s1", "CCMenuItemSpriteExtra", "p1", {40, -135}, centered);
  one->scaleX = one->scaleY = 1.2f;
  one->scaleChanged = true;
  // 2: in a box, its anchor point and z-order changed, scaled unevenly
  auto two = add("s2", "CCSprite", "p2", {190, 50}, box);
  two->scaleX = 0.5f;
  two->scaleY = 0.75f;
  two->scaleChanged = true;
  two->anchor = {0, 1};
  two->anchorChanged = true;
  two->zOrder = 3;
  two->zOrderChanged = true;
  // 3: with AnchorLayoutOptions at the bottom right, which stay
  auto three = add("s3", "CCSprite", "p3", {150, 30}, box);
  three->layout = {LayoutKind::Anchor, "AnchorLayout", true, geom::Vec2{1, 0}, std::nullopt};
  // 4: with AnchorLayoutOptions, the closest anchor
  auto four = add("s4", "CCSprite", "p4", {190, 90}, box);
  four->layout = {LayoutKind::Anchor, "AnchorLayout", true, std::nullopt, std::nullopt};
  // 5: a button in a RowLayout, scaled, with a gap before it
  auto five = add("s5", "CCMenuItemSpriteExtra", "p5", {60, 20}, box);
  five->layout = {LayoutKind::Axis, "RowLayout", false, std::nullopt, 7.5f};
  five->scaleX = five->scaleY = 1.3f;
  five->scaleChanged = true;
  // 6: in a SimpleRowLayout
  auto six = add("s6", "CCSprite", "p6", {60, 20}, box);
  six->layout = {LayoutKind::SimpleAxis, "SimpleRowLayout", false, std::nullopt, 4.f};
  six->scaleX = six->scaleY = 0.8f;
  six->scaleChanged = true;
  // 7: no ID, the third sprite in its parent
  auto seven = add("", "CCSprite", "p7", {10, 10}, screen);
  seven->typeIndex = 2;
  // 8: no ID, a class the code can't name: the fourth child of its parent
  auto eight = add("", "`anonymous namespace'::Thing", "p8", {20, 5}, screen);
  eight->byIndex = true;
  eight->typeIndex = 3;
  // 9: no ID anywhere: the lines are for `node`
  add("", "CCSprite", "", {33.25f, 12.5f}, screen);
  // 10: an ID with a quote and a backslash in it
  add("say \"hi\" \\o/", "CCSprite", "p10", {5, 5}, screen);
  // 11: a menu made bigger, with a layout of its own
  auto eleven = add("s11", "CCNode", "p11", {100, 160}, screen);
  eleven->size = {240, 40};
  eleven->sizeChanged = true;
  eleven->hasLayout = true;
  // 12: a vanilla button hidden
  auto twelve = add("s12", "CCMenuItemSpriteExtra", "p12", {50, 50}, screen);
  twelve->visible = false;
  twelve->visibleChanged = true;
  // 16 (added last): turned and faded
  // 13: no ID on it or its parent: from the layer, its second sprite, then a sprite in it by its frame
  auto thirteen = add("", "CCSprite", "", {120, 40}, screen);
  thirteen->pathFromLayer = true;
  thirteen->path = {{"", "CCSprite", 1, false, ""}, {"", "CCSprite", 0, false, "GJ_button_01.png"}};
  // 14: from a parent with an ID: a child by its ID, the third button, its first sprite
  auto fourteen = add("", "CCSprite", "", {30, 70}, box);
  fourteen->pathRoot = "p14";
  fourteen->path = {{"inner-14", "", 0, false, ""}, {"", "CCMenuItemSpriteExtra", 2, false, ""}, {"", "CCSprite", 0, false, ""}};
  // 15: an ID another node in the layer has before it: found by the path instead
  auto fifteen = add("twin", "CCSprite", "", {44, 22}, screen);
  fifteen->idUnique = false;
  fifteen->pathRoot = "p15";
  fifteen->path = {{"", "CCSprite", 1, false, ""}};
  // 16: turned and given an opacity
  auto sixteen = add("s16", "CCSprite", "p16", {70, 60}, screen);
  sixteen->rotation = -22.5f;
  sixteen->rotationChanged = true;
  sixteen->opacity = 100;
  sixteen->opacityChanged = true;
  // 17: a button made in the edit mode, in a menu with an ID, scaled
  auto seventeen = add("zhulis.ui-tweaker/new-17", "CCMenuItemSpriteExtra", "p17", {150, 60}, box);
  seventeen->zOrder = 3;
  seventeen->scaleX = seventeen->scaleY = 1.25f;
  seventeen->made.kind = "button";
  seventeen->made.frame = "GJ_infoIcon_001.png";
  seventeen->made.like = "info-button";
  seventeen->made.parent = lookup(NodeDesc{.id = "p17"}, "parent");
  // 18: a label made in the edit mode, in the layer itself
  auto eighteen = add("zhulis.ui-tweaker/new-18", "CCLabelBMFont", "", {200, 280}, screen);
  eighteen->made.kind = "label";
  eighteen->made.text = "Hello\nthere \"you\"";
  eighteen->made.font = "bigFont.fnt";
  eighteen->made.like = "a label";
  // 19: a circle button made in the edit mode
  auto nineteen = add("zhulis.ui-tweaker/new-19", "CCMenuItemSpriteExtra", "p19", {40, 40}, box);
  nineteen->made.kind = "circle button";
  nineteen->made.frame = "geode.loader/geode-logo-outline-gold.png";
  nineteen->made.baseColor = "DarkPurple";
  nineteen->made.baseSize = "MediumAlt";
  nineteen->made.like = "geode-button";
  nineteen->made.parent = lookup(NodeDesc{.id = "p19"}, "parent");
  return all;
}

// The sample without IDs that the code calls `node`
inline constexpr int NO_ID = 9;

// L's code: three buttons in a row in a parent with an ID, on the screen; three sprites in a column of the layer
// itself, in a box. Given out of order, the code puts them in order
inline std::vector<codegen::LayoutDesc> layoutSamples()
{
  using namespace codegen;
  std::vector<LayoutDesc> all;
  Frame screen{FrameKind::Screen, WIN, {0, 0}};
  Frame box{FrameKind::Parent, BOX, {}};

  LayoutDesc row;
  std::vector<geom::Rect> rowBoxes{{220, 30, 40, 40}, {100, 30, 40, 30}, {160, 30, 40, 50}};
  for (int i = 0; i < 3; i++)
  {
    NodeDesc node;
    node.id = "row-" + std::to_string(i);
    node.className = "CCMenuItemSpriteExtra";
    node.parentId = "row-parent";
    node.parentClass = "CCMenu";
    node.frame = screen;
    row.nodes.push_back(node);
  }
  row.fit = rowfit::fit(rowBoxes);
  row.menu = true;
  row.parent.id = "row-parent";
  row.parent.className = "CCMenu";
  all.push_back(row);

  LayoutDesc column;
  std::vector<geom::Rect> columnBoxes{{20, 10, 30, 20}, {20, 70, 30, 20}, {20, 40, 30, 20}};
  for (int i = 0; i < 3; i++)
  {
    NodeDesc node;
    node.id = "column-" + std::to_string(i);
    node.className = "CCSprite";
    node.parentClass = "MenuLayer";
    node.frame = box;
    column.nodes.push_back(node);
  }
  column.fit = rowfit::fit(columnBoxes);
  column.menu = false;
  column.parentIsLayer = true;
  all.push_back(column);
  return all;
}
