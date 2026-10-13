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

enum class AxisAlignment
{
  Start,
  Center,
  End,
  Even,
  Between,
};

class Layout
{
public:
  virtual ~Layout() = default;
};

class AxisLayout : public Layout
{
public:
  bool row = true;
  float gap = 5;
  AxisAlignment axisAlignment = AxisAlignment::Center;
  AxisAlignment crossLineAlignment = AxisAlignment::Center;
  bool reverse = false;
  bool autoScale = true;

  AxisLayout* setGap(float value)
  {
    gap = value;
    return this;
  }
  AxisLayout* setAxisAlignment(AxisAlignment value)
  {
    axisAlignment = value;
    return this;
  }
  AxisLayout* setCrossAxisLineAlignment(AxisAlignment value)
  {
    crossLineAlignment = value;
    return this;
  }
  AxisLayout* setAxisReverse(bool value)
  {
    reverse = value;
    return this;
  }
  AxisLayout* setAutoScale(bool value)
  {
    autoScale = value;
    return this;
  }
};

class RowLayout : public AxisLayout
{
public:
  static RowLayout* create() { return new RowLayout(); }
};

class ColumnLayout : public AxisLayout
{
public:
  static ColumnLayout* create()
  {
    auto layout = new ColumnLayout();
    layout->row = false;
    return layout;
  }
};

// GEODE_MOD_ID's prefix, as Geode's literal adds it
inline std::string operator""_spr(char const* text, size_t size)
{
  return "mod.id/" + std::string(text, size);
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
    delete layout;
  }

  static CCNode* create() { return new CCNode(); }
  void setID(std::string const& value) { id = value; }
  Layout* layout = nullptr;
  void setLayout(Layout* value)
  {
    if (value != layout)
      delete layout;
    layout = value;
  }
  bool ignoresAnchor = false;
  void ignoreAnchorPointForPosition(bool value) { ignoresAnchor = value; }
  // Held by its parent; retain and release around a move, as the generated code does, are counted
  int references = 1;
  void retain() { references++; }
  void release() { references--; }
  void removeFromParent()
  {
    if (!parent)
      return;
    auto& siblings = parent->children;
    siblings.erase(std::find(siblings.begin(), siblings.end(), this));
    parent = nullptr;
  }

  void addChild(CCNode* child)
  {
    child->parent = this;
    children.push_back(child);
  }
  void addChild(CCNode* child, int z)
  {
    addChild(child);
    child->zOrder = z;
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

  CCNode* getChildByID(std::string_view wanted)
  {
    for (auto child : children)
      if (child->id == wanted)
        return child;
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
  float rotation = 0;
  void setRotation(float value) { rotation = value; }
  void setContentSize(CCSize const& value) { size = value; }
  bool visible = true;
  void setVisible(bool value) { visible = value; }
  void updateLayout() { layoutUpdates++; }
  int layoutUpdates = 0;
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

class CCRGBAProtocol
{
public:
  virtual ~CCRGBAProtocol() = default;
  int opacity = 255;
  virtual void setOpacity(unsigned char value) { opacity = value; }
};

class CCMenu : public CCNode
{
public:
  static CCMenu* create() { return new CCMenu(); }
};

class CCSprite : public CCNode, public CCRGBAProtocol
{
public:
  // The frame it shows, for getChildBySpriteFrameName
  std::string frame;
  static CCSprite* createWithSpriteFrameName(char const* name)
  {
    auto sprite = new CCSprite();
    sprite->frame = name;
    return sprite;
  }
};

using SEL_MenuHandler = void (CCNode::*)(CCNode*);

class CCMenuItemSpriteExtra : public CCNode
{
public:
  float m_baseScale = 1;
  CCNode* image = nullptr;
  static CCMenuItemSpriteExtra* create(CCNode* sprite, CCNode*, SEL_MenuHandler)
  {
    auto button = new CCMenuItemSpriteExtra();
    button->image = sprite;
    button->addChild(sprite);
    return button;
  }
};

class ButtonSprite : public CCNode
{
public:
  std::string text;
  static ButtonSprite* create(char const* value)
  {
    auto sprite = new ButtonSprite();
    sprite->text = value;
    return sprite;
  }
};

class CCLabelBMFont : public CCNode
{
public:
  std::string text, font;
  static CCLabelBMFont* create(char const* value, char const* fnt)
  {
    auto label = new CCLabelBMFont();
    label->text = value;
    label->font = fnt;
    return label;
  }
};

namespace geode::cocos
{
  inline CCNode* getChildBySpriteFrameName(CCNode* parent, const char* name)
  {
    for (auto child : parent->children)
      if (auto sprite = dynamic_cast<CCSprite*>(child); sprite && sprite->frame == name)
        return child;
    return nullptr;
  }
} // namespace geode::cocos

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

// $modify as a plain subclass, enough for the generated hooks to compile and be called through the base
#define $modify(base) Hooked_##base : public base

class MenuLayer : public CCNode
{
public:
  virtual bool init() { return true; }
};

class LevelSelectLayer : public CCNode
{
public:
  virtual bool init(int) { return true; }
};

class PauseLayer : public CCNode
{
public:
  virtual void customSetup() {}
};

class GJGarageLayer : public CCNode
{
public:
  virtual bool init() { return true; }
};
