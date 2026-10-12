#include "Inspect.hpp"

#include "Ui.hpp"
#include "core/Expr.hpp"

#include <Geode/binding/CCScrollLayerExt.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/ui/SimpleAxisLayout.hpp>

#include <algorithm>
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
      if (any || castable(child, name))
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
         zOrder == other.zOrder;
}

inspect::NodeState inspect::stateOf(CCNode* node)
{
  return {node->getPosition(), node->getScaleX(), node->getScaleY(), node->getAnchorPoint(), node->getZOrder()};
}

void inspect::apply(CCNode* node, NodeState const& state)
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
      // Where the anchor is on the parent's box: what getAnchoredPosition gives for no offset, over the size
      auto size = parent->getContentSize();
      auto at = AnchorLayout::getAnchoredPosition(parent, options->getAnchor(), {0, 0});
      info.anchorNow = geom::Vec2{size.width > 0 ? at.x / size.width : 0.f, size.height > 0 ? at.y / size.height : 0.f};
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

  auto state = stateOf(node);
  desc.position = {state.position.x, state.position.y};
  desc.scaleX = state.scaleX;
  desc.scaleY = state.scaleY;
  desc.anchor = {state.anchor.x, state.anchor.y};
  desc.zOrder = state.zOrder;

  desc.layout = layoutOf(node);
  desc.frame = frameOf(node, desc.layout);

  if (original)
  {
    desc.scaleChanged = original->scaleX != state.scaleX || original->scaleY != state.scaleY;
    desc.anchorChanged = original->anchor != state.anchor;
    desc.zOrderChanged = original->zOrder != state.zOrder;
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
