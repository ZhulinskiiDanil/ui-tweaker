#pragma once

// Just enough of Cocos and Geode, with the same names and signatures, for the generated code to compile and run
// in a test: the nodes keep what's set on them. Checked against Geode 5's headers by hand (CCNode.h, Layout.hpp,
// SimpleAxisLayout.hpp, CCMenuItemSpriteExtra's m_baseScale)
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct CCPoint
{
  float x = 0;
  float y = 0;
};

struct CCSize
{
  float width = 0;
  float height = 0;
};

class LayoutOptions
{
public:
  virtual ~LayoutOptions() = default;
};

enum class Anchor
{
  Center,
  TopLeft,
  Top,
  TopRight,
  Right,
  BottomRight,
  Bottom,
  BottomLeft,
  Left,
};

class AnchorLayoutOptions : public LayoutOptions
{
public:
  Anchor anchor = Anchor::Center;
  CCPoint offset;

  static AnchorLayoutOptions* create() { return new AnchorLayoutOptions(); }
  AnchorLayoutOptions* setAnchor(Anchor value)
  {
    anchor = value;
    return this;
  }
  AnchorLayoutOptions* setOffset(CCPoint const& value)
  {
    offset = value;
    return this;
  }
};

class AxisLayoutOptions : public LayoutOptions
{
public:
  std::optional<float> prevGap;
  std::optional<bool> autoScale;
  // Something of its own the generated code must keep
  bool breakLine = false;

  static AxisLayoutOptions* create() { return new AxisLayoutOptions(); }
  AxisLayoutOptions* setPrevGap(std::optional<float> gap)
  {
    prevGap = gap;
    return this;
  }
  AxisLayoutOptions* setAutoScale(std::optional<bool> enabled)
  {
    autoScale = enabled;
    return this;
  }
};

template <class T, class U>
T typeinfo_cast(U* object)
{
  return dynamic_cast<T>(object);
}

class CCNode
{
public:
  std::string id;
  CCNode* parent = nullptr;
  std::vector<CCNode*> children;
  CCPoint position;
  float scaleX = 1;
  float scaleY = 1;
  CCPoint anchor{0.5f, 0.5f};
  int zOrder = 0;
  CCSize size;
  LayoutOptions* options = nullptr;

  virtual ~CCNode()
  {
    for (auto child : children)
      delete child;
    delete options;
  }

  void addChild(CCNode* child)
  {
    child->parent = this;
    children.push_back(child);
  }

  CCNode* getChildByIDRecursive(std::string_view wanted)
  {
    for (auto child : children)
    {
      if (child->id == wanted)
        return child;
      if (auto found = child->getChildByIDRecursive(wanted))
        return found;
    }
    return nullptr;
  }

  template <class T>
  T* getChildByType(int index)
  {
    int at = 0;
    for (auto child : children)
      if (auto cast = dynamic_cast<T*>(child))
        if (at++ == index)
          return cast;
    return nullptr;
  }

  CCNode* getChildByIndex(int index)
  {
    return index >= 0 && index < static_cast<int>(children.size()) ? children[index] : nullptr;
  }

  CCNode* getParent() { return parent; }
  CCSize const& getContentSize() const { return size; }
  void setPosition(CCPoint const& value) { position = value; }
  void setScale(float value) { scaleX = scaleY = value; }
  void setScaleX(float value) { scaleX = value; }
  void setScaleY(float value) { scaleY = value; }
  void setAnchorPoint(CCPoint const& value) { anchor = value; }
  void setZOrder(int value) { zOrder = value; }
  LayoutOptions* getLayoutOptions() { return options; }
  void setLayoutOptions(LayoutOptions* value, bool = true)
  {
    if (value != options)
      delete options;
    options = value;
  }
  void insertBefore(CCNode* child, CCNode* before)
  {
    child->parent = this;
    children.insert(std::find(children.begin(), children.end(), before), child);
  }
};

class CCSprite : public CCNode
{
};

class CCMenuItemSpriteExtra : public CCNode
{
public:
  float m_baseScale = 1;
};

class AxisGap : public CCNode
{
public:
  float gap = 0;
  static AxisGap* create(float value)
  {
    auto node = new AxisGap();
    node->gap = value;
    return node;
  }
};

class CCDirector
{
public:
  static CCDirector* get()
  {
    static CCDirector director;
    return &director;
  }
  CCSize getWinSize() const { return {569, 320}; }
};
