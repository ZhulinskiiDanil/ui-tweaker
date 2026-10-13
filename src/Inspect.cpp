#include "Inspect.hpp"

#include "Ui.hpp"
#include "core/Expr.hpp"
#include "core/Text.hpp"
#include "core/Hooks.hpp"

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/CCScrollLayerExt.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/SimpleAxisLayout.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <tuple>
#include <cstring>
#include <cmath>
#include <typeinfo>

using namespace geode::prelude;

namespace
{
  bool same(float a, float b)
  {
    return std::fabs(a - b) < 0.5f;
  }

  // Drawn as one thing and moved as one: a hit on a part of it is a hit on it
  bool isWhole(CCNode* node)
  {
    return typeinfo_cast<CCMenuItem*>(node) || typeinfo_cast<CCLabelBMFont*>(node) ||
           typeinfo_cast<extension::CCScale9Sprite*>(node) || typeinfo_cast<NineSlice*>(node) ||
           typeinfo_cast<CCTextInputNode*>(node);
  }

  // Holds other nodes and draws nothing of its own: hovered only when nothing else is there
  bool isContainer(CCNode* node)
  {
    // A CCLabelBMFont is a CCSpriteBatchNode, and draws
    if (typeinfo_cast<CCLayerColor*>(node) || typeinfo_cast<CCLabelBMFont*>(node))
      return false;
    return typeinfo_cast<CCLayer*>(node) || typeinfo_cast<CCScene*>(node) ||
           typeinfo_cast<CCSpriteBatchNode*>(node) || typeid(*node) == typeid(CCNode);
  }

  bool transparent(CCNode* node)
  {
    // CCNodeRGBA, CCLayerRGBA, CCSprite and CCLabelBMFont each have it from CCRGBAProtocol, without a common base
    if (auto rgba = dynamic_cast<CCRGBAProtocol*>(node))
      return rgba->getDisplayedOpacity() == 0;
    return false;
  }

  std::vector<CCNode*> sortedChildren(CCNode* node)
  {
    std::vector<CCNode*> children;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      children.push_back(child);
    // The order they're drawn in: by z, then by when they were added
    std::stable_sort(children.begin(), children.end(), [](CCNode* a, CCNode* b)
                     { return a->getZOrder() < b->getZOrder() ||
                              (a->getZOrder() == b->getZOrder() && a->getOrderOfArrival() < b->getOrderOfArrival()); });
    return children;
  }

  // A batch node with this many children holds a level's objects or particles, not interface: not looked into
  constexpr unsigned BIG_BATCH = 100;

  // Every visible node in the order it's drawn
  void drawOrder(CCNode* node, std::vector<CCNode*>& out)
  {
    if (!node->isVisible())
      return;
    if (typeinfo_cast<CCSpriteBatchNode*>(node) && !typeinfo_cast<CCLabelBMFont*>(node) &&
        node->getChildrenCount() > BIG_BATCH)
    {
      out.push_back(node);
      return;
    }
    auto children = sortedChildren(node);
    size_t i = 0;
    for (; i < children.size() && children[i]->getZOrder() < 0; i++)
      drawOrder(children[i], out);
    out.push_back(node);
    for (; i < children.size(); i++)
      drawOrder(children[i], out);
  }

  // The layout's children in its order, the invisible ones left out if it leaves them out
  std::vector<CCNode*> laidOut(CCNode* parent, bool ignoreInvisible)
  {
    std::vector<CCNode*> nodes;
    for (auto child : CCArrayExt<CCNode*>(parent->getChildren()))
      if (!ignoreInvisible || child->isVisible())
        nodes.push_back(child);
    return nodes;
  }

  std::optional<float> gapBefore(CCNode* node, bool row, bool ignoreInvisible)
  {
    auto nodes = laidOut(node->getParent(), ignoreInvisible);
    auto at = std::find(nodes.begin(), nodes.end(), node);
    if (at == nodes.end() || at == nodes.begin())
      return std::nullopt;
    auto previous = (*(at - 1))->boundingBox();
    auto self = node->boundingBox();
    if (row)
      return self.getMinX() >= previous.getMidX() ? self.getMinX() - previous.getMaxX()
                                                  : previous.getMinX() - self.getMaxX();
    return self.getMinY() >= previous.getMidY() ? self.getMinY() - previous.getMaxY()
                                                : previous.getMinY() - self.getMaxY();
  }

  // The MSVC name of the object's own class, as its RTTI has it (what typeinfo_cast compares)
  char const* rttiName(void const* object)
  {
    using namespace geode::cast;
    auto vftable = *reinterpret_cast<VftableType const* const*>(object);
    auto meta = static_cast<MetaPointerType const*>(static_cast<CompleteVftableType const*>(vftable));
    auto base = reinterpret_cast<uintptr_t>(meta->m_completeLocator) - meta->m_completeLocator->m_locatorOffset;
    return meta->m_completeLocator->m_typeDescriptor.into(base)->m_typeDescriptorName;
  }

  // typeinfo_cast with the class known only at run time: whether the object is of that class or a subclass
  bool castable(void const* object, char const* name)
  {
    using namespace geode::cast;
    auto vftable = *reinterpret_cast<VftableType const* const*>(object);
    auto meta = static_cast<MetaPointerType const*>(static_cast<CompleteVftableType const*>(vftable));
    auto base = reinterpret_cast<uintptr_t>(meta->m_completeLocator) - meta->m_completeLocator->m_locatorOffset;
    auto classDesc = meta->m_completeLocator->m_classDescriptor.into(base);
    for (int32_t i = 0; i < classDesc->m_numBaseClasses; ++i)
    {
      auto entry = classDesc->m_baseClassArray.into(base)->m_descriptorEntries[i].into(base);
      if (std::strcmp(entry->m_typeDescriptor.into(base)->m_typeDescriptorName, name) == 0)
        return true;
    }
    return false;
  }

  // Which one of the parent's children getChildByType<its class> gives the node as: subclasses count, like
  // typeinfo_cast does. Or, with `any`, which child it is of all
  // Made by Ctrl+D: its ID is this mod's new-N
  bool madeHere(CCNode* node)
  {
    static std::string const prefix = std::string(Mod::get()->getID()) + "/new-";
    return std::string_view(node->getID()).starts_with(prefix);
  }

  int childIndex(CCNode* node, bool any)
  {
    auto parent = node->getParent();
    if (!parent)
      return 0;
    auto name = rttiName(node);
    int index = 0;
    for (auto child : CCArrayExt<CCNode*>(parent->getChildren()))
    {
      if (child == node)
        break;
      // A node made in the edit mode isn't there where the code runs
      if (!madeHere(child) && (any || castable(child, name)))
        index++;
    }
    return index;
  }

  codegen::Frame frameOf(CCNode* node, codegen::LayoutInfo const& layout)
  {
    auto parent = node->getParent();
    auto win = CCDirector::get()->getWinSize();
    auto size = parent ? parent->getContentSize() : win;
    if (!parent)
      return {codegen::FrameKind::Screen, {win.width, win.height}, {}};

    // An AnchorLayout's anchors are on the parent's box
    bool anchored = layout.kind == codegen::LayoutKind::Anchor && layout.hasAnchorOptions;
    auto toWorld = inspect::toGeom(parent->nodeToWorldTransform());
    bool screenSized = (same(size.width, win.width) && same(size.height, win.height)) ||
                       (size.width == 0 && size.height == 0);
    if (!anchored && toWorld.isTranslation() && screenSized)
      return {codegen::FrameKind::Screen, {win.width, win.height}, {toWorld.tx, toWorld.ty}};
    return {codegen::FrameKind::Parent, {size.width, size.height}, {}};
  }
} // namespace

bool inspect::NodeState::operator==(NodeState const& other) const
{
  return position.x == other.position.x && position.y == other.position.y && scaleX == other.scaleX &&
         scaleY == other.scaleY && anchor.x == other.anchor.x && anchor.y == other.anchor.y &&
         zOrder == other.zOrder && size.width == other.size.width && size.height == other.size.height &&
         visible == other.visible && rotation == other.rotation && opacity == other.opacity && present == other.present;
}

inspect::NodeState inspect::stateOf(CCNode* node)
{
  auto rgba = typeinfo_cast<CCRGBAProtocol*>(node);
  return {node->getPosition(), node->getScaleX(), node->getScaleY(), node->getAnchorPoint(), node->getZOrder(),
          node->getContentSize(), node->isVisible(), node->getRotation(), rgba ? rgba->getOpacity() : 255};
}

void inspect::apply(CCNode* node, NodeState const& state, bool size, bool visible, bool turn, bool fade)
{
  node->setPosition(state.position);
  node->setScaleX(state.scaleX);
  node->setScaleY(state.scaleY);
  // A button's press animation scales it back to its base scale
  if (auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(node))
    button->m_baseScale = state.scaleX;
  if (node->getAnchorPoint() != state.anchor)
    node->setAnchorPoint(state.anchor);
  if (node->getZOrder() != state.zOrder)
    node->setZOrder(state.zOrder);
  if (visible && node->isVisible() != state.visible)
    node->setVisible(state.visible);
  // Set only when it changes: GD's skew is a rotation on each axis, which setRotation would make one
  if (turn && node->getRotation() != state.rotation)
    node->setRotation(state.rotation);
  if (auto rgba = typeinfo_cast<CCRGBAProtocol*>(node); fade && rgba && rgba->getOpacity() != state.opacity)
    rgba->setOpacity(static_cast<GLubyte>(state.opacity));
  if (size && node->getContentSize() != state.size)
  {
    node->setContentSize(state.size);
    // A menu with a layout lays its children out in its new size
    if (node->getLayout())
      node->updateLayout();
  }
}

std::string inspect::className(CCObject const* object)
{
  std::string name(getObjectName(object));
  for (std::string_view prefix : {"class ", "struct "})
    if (name.starts_with(prefix))
      name.erase(0, prefix.size());
  for (std::string_view space : {"cocos2d::extension::", "cocos2d::", "geode::"})
    if (name.starts_with(space))
      name.erase(0, space.size());
  return name;
}

std::string inspect::name(CCNode* node)
{
  std::string id(node->getID());
  if (id.empty())
    return className(node);
  return id + " (" + className(node) + ")";
}

geom::Affine inspect::toGeom(CCAffineTransform const& transform)
{
  return {transform.a, transform.b, transform.c, transform.d, transform.tx, transform.ty};
}

geom::NodeTransform inspect::transformOf(CCNode* node)
{
  geom::NodeTransform transform;
  auto position = node->getPosition();
  auto anchor = node->getAnchorPoint();
  auto size = node->getContentSize();
  transform.position = {position.x, position.y};
  transform.anchor = {anchor.x, anchor.y};
  transform.content = {size.width, size.height};
  transform.scaleX = node->getScaleX();
  transform.scaleY = node->getScaleY();
  transform.rotationX = node->getRotationX();
  transform.rotationY = node->getRotationY();
  transform.skewX = node->getSkewX();
  transform.skewY = node->getSkewY();
  transform.ignoreAnchor = node->isIgnoreAnchorPointForPosition();
  return transform;
}

geom::Quad inspect::worldQuad(CCNode* node)
{
  auto size = node->getContentSize();
  return geom::transform(toGeom(node->nodeToWorldTransform()), {0, 0, size.width, size.height});
}

bool inspect::contains(CCNode* root, CCNode const* node)
{
  if (!root || !node)
    return false;
  if (root == node)
    return true;
  for (auto child : CCArrayExt<CCNode*>(root->getChildren()))
    if (contains(child, node))
      return true;
  return false;
}

bool inspect::isIn(CCNode* node, CCNode* root)
{
  for (; node; node = node->getParent())
    if (node == root)
      return true;
  return false;
}

std::vector<CCNode*> inspect::hitsAt(CCNode* root, CCPoint world, bool deep)
{
  std::vector<CCNode*> order;
  if (root)
    drawOrder(root, order);
  geom::Vec2 point{world.x, world.y};

  std::vector<CCNode*> hits;
  auto add = [&](CCNode* node)
  {
    if (!deep)
    {
      // The outermost whole thing it's part of
      for (auto up = node->getParent(); up && up != root; up = up->getParent())
        if (isWhole(up))
          node = up;
    }
    if (std::find(hits.begin(), hits.end(), node) == hits.end())
      hits.push_back(node);
  };

  for (bool containers : {false, true})
  {
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
      auto node = *it;
      if (node == root || isContainer(node) != containers)
        continue;
      auto size = node->getContentSize();
      if (size.width <= 0 || size.height <= 0 || transparent(node))
        continue;
      if (worldQuad(node).contains(point))
        add(node);
    }
    if (!hits.empty())
      break;
  }
  return hits;
}

std::vector<CCNode*> inspect::within(CCNode* root, geom::Rect area)
{
  std::vector<CCNode*> order;
  if (root)
    drawOrder(root, order);
  auto inside = [&](CCNode* node)
  {
    auto box = worldQuad(node).bounds();
    return box.minX() >= area.minX() && box.maxX() <= area.maxX() && box.minY() >= area.minY() &&
           box.maxY() <= area.maxY();
  };

  std::vector<CCNode*> found;
  for (auto it = order.rbegin(); it != order.rend(); ++it)
  {
    auto node = *it;
    if (node == root || isContainer(node))
      continue;
    auto size = node->getContentSize();
    if (size.width <= 0 || size.height <= 0 || transparent(node))
      continue;
    // A level's objects and players (the main menu's flying icon among them) aren't interface: a box passing over
    // them doesn't take them
    bool game = false;
    for (auto up = node; up && up != root && !game; up = up->getParent())
      game = typeinfo_cast<GameObject*>(up) != nullptr;
    if (game)
      continue;
    for (auto up = node->getParent(); up && up != root; up = up->getParent())
      if (isWhole(up))
        node = up;
    if (inside(node) && std::find(found.begin(), found.end(), node) == found.end())
      found.push_back(node);
  }
  std::erase_if(found, [&](CCNode* node)
                {
                  for (auto up = node->getParent(); up; up = up->getParent())
                    if (std::find(found.begin(), found.end(), up) != found.end())
                      return true;
                  return false;
                });
  return found;
}

std::vector<CCNode*> inspect::shownWithIds(CCNode* root)
{
  std::vector<CCNode*> order;
  if (root)
    drawOrder(root, order);
  std::erase_if(order, [&](CCNode* node)
                {
                  auto size = node->getContentSize();
                  return node == root || node->getID().empty() || isContainer(node) || transparent(node) ||
                         size.width <= 0 || size.height <= 0;
                });
  return order;
}

codegen::LayoutInfo inspect::layoutOf(CCNode* node)
{
  codegen::LayoutInfo info;
  auto parent = node->getParent();
  auto layout = parent ? parent->getLayout() : nullptr;
  if (!layout)
    return info;
  info.name = className(layout);

  // A CopySizeLayout is an AnchorLayout too: it moves the children with AnchorLayoutOptions (a popup's main layer)
  if (typeinfo_cast<AnchorLayout*>(layout))
  {
    info.kind = codegen::LayoutKind::Anchor;
    if (auto options = typeinfo_cast<AnchorLayoutOptions*>(node->getLayoutOptions()))
    {
      info.hasAnchorOptions = true;
      // Where the anchor is on the parent's box, as a share of it on each axis
      switch (options->getAnchor())
      {
      case Anchor::Center: info.anchorNow = geom::Vec2{0.5f, 0.5f}; break;
      case Anchor::TopLeft: info.anchorNow = geom::Vec2{0, 1}; break;
      case Anchor::Top: info.anchorNow = geom::Vec2{0.5f, 1}; break;
      case Anchor::TopRight: info.anchorNow = geom::Vec2{1, 1}; break;
      case Anchor::Right: info.anchorNow = geom::Vec2{1, 0.5f}; break;
      case Anchor::BottomRight: info.anchorNow = geom::Vec2{1, 0}; break;
      case Anchor::Bottom: info.anchorNow = geom::Vec2{0.5f, 0}; break;
      case Anchor::BottomLeft: info.anchorNow = geom::Vec2{0, 0}; break;
      case Anchor::Left: info.anchorNow = geom::Vec2{0, 0.5f}; break;
      }
    }
  }
  else if (auto axis = typeinfo_cast<AxisLayout*>(layout))
  {
    info.kind = codegen::LayoutKind::Axis;
    info.prevGap = gapBefore(node, axis->getAxis() == Axis::Row, axis->isIgnoreInvisibleChildren());
  }
  else if (auto simple = typeinfo_cast<SimpleAxisLayout*>(layout))
  {
    // Its options aren't AxisLayoutOptions, its gaps are AxisGap nodes
    info.kind = codegen::LayoutKind::SimpleAxis;
    info.prevGap = gapBefore(node, simple->getAxis() == Axis::Row, simple->isIgnoreInvisibleChildren());
  }
  else
    info.kind = codegen::LayoutKind::Other;
  return info;
}

namespace
{
  // The name the sprite's frame is cached under, if it's one
  std::string frameName(CCSprite* sprite)
  {
    auto frames = CCSpriteFrameCache::get()->m_pSpriteFrames;
    if (!sprite || !frames)
      return "";
    // Every frame by texture and rect, made again when frames come or go (a reload makes them all again): one
    // look-up a sprite instead of a pass over thousands of frames
    using Key = std::tuple<void*, float, float, float, float>;
    static std::map<Key, std::string> names;
    static unsigned built = ~0u;
    if (frames->count() != built)
    {
      names.clear();
      // The frame's texture as it is, not getTexture(), which loads a texture not loaded yet
      for (auto [name, frame] : CCDictionaryExt<std::string_view, CCSpriteFrame*>(frames))
      {
        auto rect = frame->getRect();
        names.try_emplace(Key{frame->m_pobTexture, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height},
                          std::string(name));
      }
      built = frames->count();
    }
    auto rect = sprite->getTextureRect();
    auto found = names.find(Key{sprite->getTexture(), rect.origin.x, rect.origin.y, rect.size.width, rect.size.height});
    return found != names.end() ? found->second : "";
  }

  // The same, remembered by texture and rect: the code view describes the selected node every frame, and a path
  // down to it can go through several sprites. Forgotten as it grows (and textures are made again on a reload)
  std::string cachedFrameName(CCSprite* sprite)
  {
    static std::map<std::tuple<void*, float, float, float, float>, std::string> cache;
    if (!sprite)
      return "";
    auto rect = sprite->getTextureRect();
    std::tuple<void*, float, float, float, float> key{sprite->getTexture(), rect.origin.x, rect.origin.y,
                                                      rect.size.width, rect.size.height};
    if (auto found = cache.find(key); found != cache.end())
      return found->second;
    if (cache.size() > 4096)
      cache.clear();
    return cache[key] = frameName(sprite);
  }

  // One step of the way down to a node without an ID: by its frame if none of the siblings before it has it,
  // else by its class and how many of its class come before it
  codegen::NodeDesc::PathStep stepTo(CCNode* node)
  {
    codegen::NodeDesc::PathStep step;
    // By its ID if it's the first child with it
    if (!node->getID().empty() && node->getParent() && node->getParent()->getChildByID(node->getID()) == node)
    {
      step.id = std::string(node->getID());
      return step;
    }
    step.className = inspect::className(node);
    step.byIndex = step.className.find_first_of(":`<> ") != std::string::npos;
    step.typeIndex = childIndex(node, step.byIndex);
    std::string frame;
    if (auto button = typeinfo_cast<CCMenuItemSprite*>(node))
      frame = cachedFrameName(typeinfo_cast<CCSprite*>(button->getNormalImage()));
    else if (auto sprite = typeinfo_cast<CCSprite*>(node))
      frame = cachedFrameName(sprite);
    if (!frame.empty() && node->getParent())
      for (auto sibling : CCArrayExt<CCNode*>(node->getParent()->getChildren()))
        if (geode::cocos::isSpriteFrameName(sibling, frame.c_str()))
        {
          if (sibling == node)
            step.frameName = frame;
          break;
        }
    return step;
  }
} // namespace

namespace
{
  std::string oneLine(char const* text)
  {
    std::string out = text ? text : "";
    std::replace(out.begin(), out.end(), char(10), ' ');
    return out;
  }
} // namespace

std::string inspect::lookOf(CCNode* node)
{
  if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
    return fmt::format("{}: \"{}\"", label->getFntFile() ? label->getFntFile() : "?",
                       text::fit(oneLine(label->getString()), 24));
  if (auto label = typeinfo_cast<CCLabelTTF*>(node))
    return fmt::format("{}: \"{}\"", label->getFontName() ? label->getFontName() : "?",
                       text::fit(oneLine(label->getString()), 24));
  // A button shows its sprite, or its label
  if (auto button = typeinfo_cast<CCMenuItemSprite*>(node))
    return button->getNormalImage() ? lookOf(button->getNormalImage()) : "";
  if (auto sprite = typeinfo_cast<CCSprite*>(node))
    return cachedFrameName(sprite);
  return "";
}

explain::Facts inspect::factsOf(CCNode* node)
{
  explain::Facts facts;
  auto parent = node->getParent();
  facts.parent = parent ? name(parent) : "nothing";

  auto layout = layoutOf(node);
  if (layout.kind != codegen::LayoutKind::None)
  {
    facts.layoutName = layout.name;
    facts.layoutMovesIt = layout.kind != codegen::LayoutKind::Anchor || layout.hasAnchorOptions;
  }
  for (auto up = node; up; up = up->getParent())
    if (!up->isVisible())
    {
      facts.hiddenBy = up == node ? ui::words().itself : name(up);
      break;
    }
  facts.transparent = transparent(node);
  auto size = node->getContentSize();
  facts.zeroSize = size.width <= 0 && size.height <= 0;
  facts.zeroScale = node->getScaleX() == 0 || node->getScaleY() == 0;
  facts.ignoresAnchor = node->isIgnoreAnchorPointForPosition() && node->getAnchorPoint() != CCPoint{0, 0};
  facts.runningActions = node->numberOfRunningActions();

  auto bounds = worldQuad(node).bounds();
  for (auto up = parent; up; up = up->getParent())
    if (typeinfo_cast<CCClippingNode*>(up) || typeinfo_cast<CCScrollLayerExt*>(up))
    {
      auto box = worldQuad(up).bounds();
      bool inside = bounds.minX() >= box.minX() - 0.5f && bounds.maxX() <= box.maxX() + 0.5f &&
                    bounds.minY() >= box.minY() - 0.5f && bounds.maxY() <= box.maxY() + 0.5f;
      if (!inside)
        facts.clippedBy = name(up);
      break;
    }

  if (parent)
  {
    auto toWorld = toGeom(parent->nodeToWorldTransform());
    // Scaled evenly is fine, turned, flipped or skewed isn't
    facts.parentTurned = std::fabs(toWorld.b) > 1e-4f || std::fabs(toWorld.c) > 1e-4f || toWorld.a < 0 || toWorld.d < 0;
  }
  auto win = CCDirector::get()->getWinSize();
  facts.outsideScreen = !facts.zeroSize && (bounds.maxX() < 0 || bounds.minX() > win.width || bounds.maxY() < 0 ||
                                            bounds.minY() > win.height);
  return facts;
}

namespace
{
  // Whether the object is of the class or a subclass, by the class's plain name ("CCSprite"), as getChildByType
  // counts: its RTTI's base classes, ".?AVCCSprite@cocos2d@@" being CCSprite
  bool ofClass(void const* object, std::string const& name)
  {
    using namespace geode::cast;
    auto vftable = *reinterpret_cast<VftableType const* const*>(object);
    auto meta = static_cast<MetaPointerType const*>(static_cast<CompleteVftableType const*>(vftable));
    auto base = reinterpret_cast<uintptr_t>(meta->m_completeLocator) - meta->m_completeLocator->m_locatorOffset;
    auto classDesc = meta->m_completeLocator->m_classDescriptor.into(base);
    for (int32_t i = 0; i < classDesc->m_numBaseClasses; ++i)
    {
      auto entry = classDesc->m_baseClassArray.into(base)->m_descriptorEntries[i].into(base);
      std::string_view raw = entry->m_typeDescriptor.into(base)->m_typeDescriptorName;
      if (raw.starts_with(".?AV") || raw.starts_with(".?AU"))
        raw.remove_prefix(4);
      if (raw.substr(0, raw.find('@')) == name)
        return true;
    }
    return false;
  }

  // The n-th child getChildByType<T> would give, T named
  CCNode* childOfClass(CCNode* parent, std::string const& name, int index)
  {
    int seen = 0;
    for (auto child : CCArrayExt<CCNode*>(parent->getChildren()))
      if (!madeHere(child) && ofClass(child, name) && seen++ == index)
        return child;
    return nullptr;
  }
} // namespace

CCNode* inspect::resolve(CCNode* scene, codegen::NodeDesc const& desc)
{
  if (!scene)
    return nullptr;
  // The layer the code's hook is in: the first node of its class, breadth first
  CCNode* layer = nullptr;
  std::vector<CCNode*> queue{scene};
  for (size_t i = 0; i < queue.size() && !layer && !desc.layer.empty(); i++)
  {
    if (queue[i] != scene && className(queue[i]) == desc.layer)
      layer = queue[i];
    for (auto child : CCArrayExt<CCNode*>(queue[i]->getChildren()))
      queue.push_back(child);
  }
  // Of a layer that isn't here: nothing (another layer's node with the same ID isn't it)
  if (!desc.layer.empty() && !layer)
    return nullptr;
  auto from = layer ? layer : scene;

  CCNode* found = nullptr;
  if (!desc.id.empty() && (desc.idUnique || desc.path.empty()))
    found = from->getChildByIDRecursive(desc.id);
  else if (!desc.path.empty())
  {
    CCNode* at = desc.pathFromLayer ? layer : from->getChildByIDRecursive(desc.pathRoot);
    for (size_t i = 0; at && i < desc.path.size(); i++)
    {
      auto const& step = desc.path[i];
      if (!step.id.empty())
        at = at->getChildByID(step.id);
      else if (!step.frameName.empty())
        at = geode::cocos::getChildBySpriteFrameName(at, step.frameName.c_str());
      else if (step.byIndex)
        at = at->getChildByIndex(step.typeIndex);
      else
        at = childOfClass(at, step.className, step.typeIndex);
    }
    found = at;
  }
  else if (!desc.parentId.empty())
  {
    if (auto parent = from->getChildByIDRecursive(desc.parentId))
      found = desc.byIndex ? parent->getChildByIndex(desc.typeIndex) : childOfClass(parent, desc.className, desc.typeIndex);
  }
  // Of the class it was: a path that leads to something else now leads nowhere
  return found && className(found) == desc.className ? found : nullptr;
}

CCNode* inspect::followPath(CCNode* node, codegen::NodeDesc const& desc)
{
  if (desc.path.empty())
    return nullptr;
  // The layer the hook is in, `this`: the closest parent of that class
  CCNode* layer = nullptr;
  for (auto up = node->getParent(); up && !layer; up = up->getParent())
    if (className(up) == desc.layer)
      layer = up;
  if (!layer)
    return nullptr;
  CCNode* at = desc.pathFromLayer ? layer : layer->getChildByIDRecursive(desc.pathRoot);
  // A node in a hook's layer: getChildByIDRecursive from the layer
  // The nodes the steps are expected to reach, for the class a getChildByType would be given
  std::vector<CCNode*> chain;
  for (auto up = node; chain.size() < desc.path.size() && up; up = up->getParent())
    chain.insert(chain.begin(), up);
  if (chain.size() < desc.path.size())
    return nullptr;
  for (size_t i = 0; at && i < desc.path.size(); i++)
  {
    auto const& step = desc.path[i];
    if (!step.id.empty())
      at = at->getChildByID(step.id);
    else if (!step.frameName.empty())
      at = geode::cocos::getChildBySpriteFrameName(at, step.frameName.c_str());
    else if (step.byIndex)
      at = at->getChildByIndex(step.typeIndex);
    else
    {
      // getChildByType<T>(n): the n-th child typeinfo_cast<T*> takes
      auto name = rttiName(chain[i]);
      CCNode* found = nullptr;
      int seen = 0;
      for (auto child : CCArrayExt<CCNode*>(at->getChildren()))
        if (!madeHere(child) && castable(child, name) && seen++ == step.typeIndex)
        {
          found = child;
          break;
        }
      at = found;
    }
  }
  return at;
}

namespace
{
  struct Lookup
  {
    std::string id;
    CCNode* parent = nullptr;
    bool idUnique = true;
    std::string pathRoot;
    bool pathFromLayer = false;
    std::vector<codegen::NodeDesc::PathStep> path;
    double time = 0;
  };
  // By node, checked against its ID and parent; nothing in it is used as a node
  std::map<CCNode*, Lookup> g_lookups;

  double secondsNow()
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  // An ID getChildByIDRecursive finds from the layer, not another node's that's the same; without an ID (or with
  // one another node has too) the way down from the closest parent with a findable one, or from the layer (`this` in
  // its hook; its own ID, which node-ids sets to its class name, isn't among its children)
  void findLookup(CCNode* node, CCNode* layer, codegen::NodeDesc& desc)
  {
    auto findable = [&](CCNode* target)
    {
      auto from = layer ? layer : static_cast<CCNode*>(CCDirector::get()->getRunningScene());
      return !target->getID().empty() && from && from->getChildByIDRecursive(target->getID()) == target;
    };
    desc.idUnique = desc.id.empty() || findable(node);
    if (desc.idUnique && (!desc.id.empty() || !node->getParent()))
      return;
    std::vector<CCNode*> chain{node};
    auto up = node->getParent();
    while (up && up != layer && !findable(up) && !typeinfo_cast<CCScene*>(up))
    {
      chain.push_back(up);
      up = up->getParent();
    }
    if (up && (up == layer || (findable(up) && !typeinfo_cast<CCScene*>(up))))
    {
      desc.pathFromLayer = up == layer;
      desc.pathRoot = desc.pathFromLayer ? "" : std::string(up->getID());
      for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        desc.path.push_back(stepTo(*it));
    }
  }
} // namespace

std::optional<codegen::NodeDesc::Made> inspect::madeLike(CCNode* node)
{
  codegen::NodeDesc::Made made;
  made.like = name(node);
  if (auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(node))
  {
    auto image = button->getNormalImage();
    // A ButtonSprite is a sprite too: its text first
    if (auto text = typeinfo_cast<ButtonSprite*>(image); text && text->m_label)
    {
      made.kind = "text button";
      made.text = text->m_label->getString();
      return made;
    }
    // Geode's circle button: its base's frame is geode.loader/baseCircle_<size>_<color>.png, named after the enums;
    // its top a plain sprite
    if (auto circle = typeinfo_cast<geode::CircleButtonSprite*>(image))
    {
      auto base = cachedFrameName(circle);
      auto top = typeinfo_cast<CCSprite*>(circle->getTopNode());
      auto at = base.find("baseCircle_");
      if (at == std::string::npos || !top || className(top) != "CCSprite")
        return std::nullopt;
      auto names = base.substr(at + std::string_view("baseCircle_").size());
      names = names.substr(0, names.rfind(".png"));
      auto split = names.find('_');
      if (split == std::string::npos)
        return std::nullopt;
      made.baseSize = names.substr(0, split);
      made.baseColor = names.substr(split + 1);
      made.frame = cachedFrameName(top);
      if (made.frame.empty())
        return std::nullopt;
      made.kind = "circle button";
      return made;
    }
    // A plain sprite only: a sprite of its own class (a circle button with an icon) isn't made by its frame
    if (auto sprite = typeinfo_cast<CCSprite*>(image); sprite && className(image) == "CCSprite")
      made.frame = cachedFrameName(sprite);
    if (made.frame.empty())
      return std::nullopt;
    made.kind = "button";
    return made;
  }
  if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
  {
    made.kind = "label";
    made.text = label->getString();
    made.font = label->getFntFile() ? label->getFntFile() : "";
    return made.font.empty() ? std::nullopt : std::optional(made);
  }
  if (auto label = typeinfo_cast<CCLabelTTF*>(node))
  {
    made.kind = "ttf";
    made.text = label->getString();
    made.font = label->getFontName() ? label->getFontName() : "";
    made.fontSize = label->getFontSize();
    return made;
  }
  // A plain sprite (a class of its own wouldn't be made by createWithSpriteFrameName)
  if (auto sprite = typeinfo_cast<CCSprite*>(node); sprite && className(node) == "CCSprite")
  {
    made.frame = cachedFrameName(sprite);
    if (made.frame.empty())
      return std::nullopt;
    made.kind = "sprite";
    return made;
  }
  return std::nullopt;
}

CCNode* inspect::make(codegen::NodeDesc::Made const& made)
{
  if (made.kind == "sprite")
    return CCSprite::createWithSpriteFrameName(made.frame.c_str());
  if (made.kind == "button")
    return CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName(made.frame.c_str()), nullptr, nullptr);
  if (made.kind == "text button")
    return CCMenuItemSpriteExtra::create(ButtonSprite::create(made.text.c_str()), nullptr, nullptr);
  if (made.kind == "circle button")
  {
    // The enums by their names, in their order
    static char const* const sizes[] = {"Tiny", "Small", "SmallAlt", "Medium", "MediumAlt", "Big", "BigAlt", "Large"};
    static char const* const colors[] = {"Green", "Pink", "Gray", "Blue", "Cyan", "DarkPurple", "DarkAqua", "Red"};
    auto index = [](auto const& names, std::string const& name)
    {
      for (size_t i = 0; i < std::size(names); i++)
        if (name == names[i])
          return static_cast<int>(i);
      return -1;
    };
    int size = index(sizes, made.baseSize), color = index(colors, made.baseColor);
    if (size < 0 || color < 0)
      return nullptr;
    auto sprite = geode::CircleButtonSprite::createWithSpriteFrameName(made.frame.c_str(), 1.f,
                                                                       static_cast<geode::CircleBaseColor>(color),
                                                                       static_cast<geode::CircleBaseSize>(size));
    return sprite ? CCMenuItemSpriteExtra::create(sprite, nullptr, nullptr) : nullptr;
  }
  if (made.kind == "label")
    return CCLabelBMFont::create(made.text.c_str(), made.font.c_str());
  if (made.kind == "ttf")
    return CCLabelTTF::create(made.text.c_str(), made.font.c_str(), made.fontSize);
  return nullptr;
}

void inspect::forgetLookups()
{
  g_lookups.clear();
}

codegen::NodeDesc inspect::describe(CCNode* node, NodeState const* original)
{
  codegen::NodeDesc desc;
  desc.id = std::string(node->getID());
  desc.className = className(node);
  if (auto parent = node->getParent())
  {
    desc.parentId = std::string(parent->getID());
    desc.parentClass = className(parent);
  }
  // A class from another mod, in a namespace or a template can't be named in the code: the child's index instead
  desc.byIndex = desc.className.find_first_of(":`<> ") != std::string::npos;
  desc.typeIndex = childIndex(node, desc.byIndex);

  // The hook the code goes in: the closest parent that's a layer with a known hook, else the top one under the
  // scene (a popup, a layer of a mod)
  CCNode* top = nullptr;
  CCNode* layer = nullptr;
  for (auto up = node->getParent(); up; up = up->getParent())
  {
    auto name = className(up);
    if (hooks::find(name))
    {
      desc.layer = name;
      layer = up;
      break;
    }
    if (up->getParent() && typeinfo_cast<CCScene*>(up->getParent()))
      top = up;
  }
  if (desc.layer.empty() && top)
  {
    desc.layer = className(top);
    layer = top;
    desc.layerIsPopup = typeinfo_cast<geode::Popup*>(top) != nullptr;
  }

  // How the code finds it: searching the layer for IDs is slow in a level with thousands of objects, and the panel
  // describes nodes every frame, so it's remembered for a while
  if (auto found = g_lookups.find(node); found != g_lookups.end() && found->second.id == desc.id &&
                                         found->second.parent == node->getParent() && secondsNow() - found->second.time < 0.5)
  {
    desc.idUnique = found->second.idUnique;
    desc.pathRoot = found->second.pathRoot;
    desc.pathFromLayer = found->second.pathFromLayer;
    desc.path = found->second.path;
  }
  else
  {
    findLookup(node, layer, desc);
    if (g_lookups.size() > 256)
      g_lookups.clear();
    g_lookups[node] = {desc.id, node->getParent(), desc.idUnique, desc.pathRoot, desc.pathFromLayer, desc.path, secondsNow()};
  }

  auto state = stateOf(node);
  desc.position = {state.position.x, state.position.y};
  desc.scaleX = state.scaleX;
  desc.scaleY = state.scaleY;
  desc.anchor = {state.anchor.x, state.anchor.y};
  desc.zOrder = state.zOrder;
  desc.size = {state.size.width, state.size.height};
  desc.hasLayout = node->getLayout() != nullptr;
  desc.visible = state.visible;
  desc.rotation = state.rotation;
  desc.opacity = state.opacity;

  desc.layout = layoutOf(node);
  desc.frame = frameOf(node, desc.layout);

  if (original)
  {
    desc.scaleChanged = original->scaleX != state.scaleX || original->scaleY != state.scaleY;
    desc.anchorChanged = original->anchor != state.anchor;
    desc.zOrderChanged = original->zOrder != state.zOrder;
    desc.sizeChanged = original->size != state.size;
    desc.visibleChanged = original->visible != state.visible;
    desc.rotationChanged = original->rotation != state.rotation;
    desc.opacityChanged = original->opacity != state.opacity;
  }
  return desc;
}

std::string inspect::checkCode(CCNode* node, std::string const& code)
{
  auto parent = node->getParent();
  auto win = CCDirector::get()->getWinSize();
  auto parentSize = parent ? parent->getContentSize() : win;
  auto lookup = [&](std::string_view name) -> std::optional<float>
  {
    if (name == "winSize.width")
      return win.width;
    if (name == "winSize.height")
      return win.height;
    if (name == "parentSize.width")
      return parentSize.width;
    if (name == "parentSize.height")
      return parentSize.height;
    return std::nullopt;
  };
  auto position = node->getPosition();
  auto compare = [&](float x, float y, char const* what) -> std::string
  {
    if (std::fabs(x - position.x) > 0.01f || std::fabs(y - position.y) > 0.01f)
      return fmt::format("{} gives ({}, {}), the node is at ({}, {})", what, x, y, position.x, position.y);
    return "";
  };

  if (auto offset = expr::callArguments(code, "setOffset"); offset.size() == 2)
  {
    auto anchor = expr::callArguments(code, "setAnchor");
    auto x = expr::evaluate(offset[0], lookup);
    auto y = expr::evaluate(offset[1], lookup);
    if (anchor.size() != 1 || !x || !y || !parent)
      return "can't read the AnchorLayoutOptions";
    static std::pair<char const*, Anchor> const anchors[] = {
        {"Center", Anchor::Center}, {"TopLeft", Anchor::TopLeft}, {"Top", Anchor::Top},
        {"TopRight", Anchor::TopRight}, {"Right", Anchor::Right}, {"BottomRight", Anchor::BottomRight},
        {"Bottom", Anchor::Bottom}, {"BottomLeft", Anchor::BottomLeft}, {"Left", Anchor::Left},
    };
    for (auto const& [text, value] : anchors)
      if (anchor[0] == std::string("Anchor::") + text)
      {
        auto at = AnchorLayout::getAnchoredPosition(parent, value, {*x, *y});
        return compare(at.x, at.y, "the anchor and the offset");
      }
    return "unknown anchor " + anchor[0];
  }

  auto args = expr::callArguments(code, "setPosition");
  if (args.size() != 2)
    return "no setPosition in the code";
  auto x = expr::evaluate(args[0], lookup);
  auto y = expr::evaluate(args[1], lookup);
  if (!x || !y)
    return "can't read setPosition(" + args[0] + ", " + args[1] + ")";
  return compare(*x, *y, "setPosition");
}
