// The generated code, compiled against FakeGeode.hpp and run on fake nodes: each node has to end up where its
// sample says, with its scale, anchor point, z-order and layout options
#include "FakeGeode.hpp"

#include "../Check.hpp"
#include "Samples.hpp"

#include <cstdio>

namespace
{
  std::vector<CCNode*> g_nodes;
  std::vector<CCNode*> g_parents;

  CCNode* make(std::string const& className)
  {
    if (className == "CCMenuItemSpriteExtra")
      return new CCMenuItemSpriteExtra();
    if (className == "CCSprite")
      return new CCSprite();
    return new CCNode();
  }

  geom::Vec2 anchorK(Anchor anchor)
  {
    switch (anchor)
    {
    case Anchor::Center: return {0.5f, 0.5f};
    case Anchor::TopLeft: return {0, 1};
    case Anchor::Top: return {0.5f, 1};
    case Anchor::TopRight: return {1, 1};
    case Anchor::Right: return {1, 0.5f};
    case Anchor::BottomRight: return {1, 0};
    case Anchor::Bottom: return {0.5f, 0};
    case Anchor::BottomLeft: return {0, 0};
    case Anchor::Left: return {0, 0.5f};
    }
    return {};
  }

  bool close(float a, float b)
  {
    return near(a, b, 0.006f);
  }
} // namespace

// A sample found by a path: the nodes on the way, each after siblings its step has to tell it from (others of its
// class, a sprite with another frame, a child with another ID), and before all of it a node with the sample's own
// ID when another node is meant to have it too
void buildPath(CCNode& layer, codegen::NodeDesc const& desc)
{
  if (!desc.id.empty() && !desc.idUnique)
  {
    auto twin = new CCSprite();
    twin->id = desc.id;
    layer.addChild(twin);
  }
  CCNode* at = &layer;
  if (!desc.pathFromLayer)
  {
    at = new CCNode();
    at->id = desc.pathRoot;
    layer.addChild(at);
  }
  CCNode* node = nullptr;
  for (auto const& step : desc.path)
  {
    if (!step.id.empty())
    {
      auto other = new CCNode();
      other->id = step.id + "-not";
      at->addChild(other);
      node = new CCNode();
      node->id = step.id;
    }
    else if (!step.frameName.empty())
    {
      auto other = new CCSprite();
      other->frame = "another.png";
      at->addChild(other);
      auto sprite = new CCSprite();
      sprite->frame = step.frameName;
      node = sprite;
    }
    else
    {
      // Others of its class before it, and a node of another class in between that isn't counted
      bool button = step.className == "CCMenuItemSpriteExtra";
      for (int i = 0; i < step.typeIndex; i++)
      {
        if (!button)
          at->addChild(new CCMenuItemSpriteExtra());
        at->addChild(make(step.className));
      }
      if (!button)
        at->addChild(new CCMenuItemSpriteExtra());
      node = make(step.className);
    }
    at->addChild(node);
    at = node;
  }
  if (!desc.id.empty())
    node->id = desc.id;
  // The node's own frame box for samples in one
  if (auto parent = node->getParent(); parent && desc.frame.kind == codegen::FrameKind::Parent)
    parent->size = {desc.frame.extent.width, desc.frame.extent.height};
  g_nodes.push_back(node);
  g_parents.push_back(node->getParent());
}

// Every sample's parent and node in the layer, with the siblings the lookups count
void buildScene(CCNode& layer)
{
  g_nodes.clear();
  g_parents.clear();
  for (auto const& desc : samples())
  {
    if (!desc.path.empty())
    {
      buildPath(layer, desc);
      continue;
    }
    // Made by the code: only its parent is there (the layer itself for one without a parent ID), the node found
    // after the code ran
    if (!desc.made.kind.empty())
    {
      CCNode* parent = &layer;
      if (!desc.parentId.empty())
      {
        parent = new CCMenu();
        parent->id = desc.parentId;
        parent->size = desc.frame.kind == codegen::FrameKind::Parent ? CCSize{desc.frame.extent.width, desc.frame.extent.height}
                                                                      : CCSize{WIN.width, WIN.height};
        layer.addChild(parent);
      }
      else
        layer.size = {WIN.width, WIN.height};
      g_nodes.push_back(nullptr);
      g_parents.push_back(parent);
      continue;
    }
    auto parent = new CCNode();
    parent->id = desc.parentId;
    parent->size = desc.frame.kind == codegen::FrameKind::Parent ? CCSize{desc.frame.extent.width, desc.frame.extent.height}
                                                                  : CCSize{WIN.width, WIN.height};
    layer.addChild(parent);
    // Siblings before it: of its class for getChildByType (and a button in between, which isn't counted), any for
    // getChildByIndex
    for (int i = 0; i < desc.typeIndex; i++)
    {
      if (!desc.byIndex)
        parent->addChild(new CCMenuItemSpriteExtra());
      parent->addChild(desc.byIndex ? new CCNode() : make(desc.className));
    }
    if (!desc.byIndex)
      while (static_cast<int>(parent->children.size()) > desc.typeIndex * 2)
        parent->children.pop_back();
    auto node = make(desc.className);
    node->id = desc.id;
    if (desc.layout.kind == codegen::LayoutKind::Anchor)
    {
      auto options = AnchorLayoutOptions::create();
      options->anchor = Anchor::TopLeft;
      if (desc.layout.anchorNow && desc.layout.anchorNow->x == 1 && desc.layout.anchorNow->y == 0)
        options->anchor = Anchor::BottomRight;
      node->options = options;
    }
    if (desc.layout.kind == codegen::LayoutKind::Axis)
    {
      auto options = AxisLayoutOptions::create();
      options->breakLine = true;
      node->options = options;
    }
    parent->addChild(node);
    g_nodes.push_back(node);
    g_parents.push_back(parent);
  }
}

CCNode* nodeOf(int index)
{
  return g_nodes[index];
}

void checkSample(int index)
{
  // A copy: samples() makes a new vector, a reference into it would dangle
  auto const all = samples();
  auto const& desc = all[index];
  auto node = g_nodes[index];
  if (!desc.made.kind.empty())
  {
    // Made by the code, in its parent, with the ID the mod's prefix gives it
    auto suffix = desc.id.substr(desc.id.find('/') + 1);
    node = g_parents[index]->getChildByID("mod.id/" + suffix);
    std::printf("sample %d: made %s\n", index, node ? "yes" : "no");
    CHECK(node != nullptr);
    if (!node)
      return;
    CHECK(node->zOrder == desc.zOrder);
    if (desc.made.kind == "button")
    {
      auto button = dynamic_cast<CCMenuItemSpriteExtra*>(node);
      auto sprite = button ? dynamic_cast<CCSprite*>(button->image) : nullptr;
      CHECK(sprite && sprite->frame == desc.made.frame);
    }
    if (desc.made.kind == "circle button")
    {
      auto button = dynamic_cast<CCMenuItemSpriteExtra*>(node);
      auto circle = button ? dynamic_cast<CircleButtonSprite*>(button->image) : nullptr;
      CHECK(circle && circle->top == desc.made.frame && circle->color == CircleBaseColor::DarkPurple &&
            circle->size == CircleBaseSize::MediumAlt);
    }
    if (desc.made.kind == "label")
    {
      auto label = dynamic_cast<CCLabelBMFont*>(node);
      CHECK(label && label->text == desc.made.text && label->font == desc.made.font);
    }
  }
  auto layout = desc.layout.kind;
  std::printf("sample %d: (%.2f, %.2f) scale %.2f x %.2f\n", index, node->position.x, node->position.y, node->scaleX,
              node->scaleY);

  if (layout == codegen::LayoutKind::Axis)
  {
    // The position is the layout's: untouched. Its own options kept, the gap and the auto scale set on them
    CHECK(close(node->position.x, 0) && close(node->position.y, 0));
    auto options = dynamic_cast<AxisLayoutOptions*>(node->options);
    CHECK(options && options->breakLine);
    CHECK(options && options->prevGap && close(*options->prevGap, *desc.layout.prevGap));
    CHECK(options && options->autoScale == std::optional<bool>(false));
  }
  else if (layout == codegen::LayoutKind::SimpleAxis)
  {
    CHECK(close(node->position.x, 0) && close(node->position.y, 0));
    CHECK(node->options == nullptr);
  }
  else if (layout == codegen::LayoutKind::Anchor)
  {
    // Where AnchorLayout would put it: the anchor on the parent's box plus the offset
    auto options = dynamic_cast<AnchorLayoutOptions*>(node->options);
    CHECK(options != nullptr);
    if (options)
    {
      auto k = anchorK(options->anchor);
      auto size = node->parent->size;
      CHECK(close(k.x * size.width + options->offset.x, desc.position.x));
      CHECK(close(k.y * size.height + options->offset.y, desc.position.y));
      if (desc.layout.anchorNow)
        CHECK(close(k.x, desc.layout.anchorNow->x) && close(k.y, desc.layout.anchorNow->y));
    }
  }
  else
  {
    CHECK(close(node->position.x, desc.position.x));
    CHECK(close(node->position.y, desc.position.y));
  }

  if (desc.scaleChanged || desc.scaleX != 1 || desc.scaleY != 1)
  {
    CHECK(close(node->scaleX, desc.scaleX));
    CHECK(close(node->scaleY, desc.scaleY));
    if (auto button = dynamic_cast<CCMenuItemSpriteExtra*>(node))
      CHECK(close(button->m_baseScale, desc.scaleX));
  }
  if (desc.anchorChanged)
    CHECK(close(node->anchor.x, desc.anchor.x) && close(node->anchor.y, desc.anchor.y));
  if (desc.zOrderChanged)
    CHECK(node->zOrder == desc.zOrder);
  if (desc.visibleChanged)
    CHECK(node->visible == desc.visible);
  if (desc.rotationChanged)
    CHECK(close(node->rotation, desc.rotation));
  if (desc.opacityChanged)
  {
    auto rgba = dynamic_cast<CCRGBAProtocol*>(node);
    CHECK(rgba && rgba->opacity == desc.opacity);
  }
  if (desc.sizeChanged)
  {
    CHECK(close(node->size.width, desc.size.width) && close(node->size.height, desc.size.height));
    CHECK(node->layoutUpdates == (desc.hasLayout ? 1 : 0));
  }
}

// A layout sample's nodes in their parent (one with its ID in the layer, or the layer itself), last first: the code
// has to put them in order
std::vector<CCNode*> g_layoutNodes;

void buildLayoutScene(CCNode& layer, int index)
{
  auto const all = layoutSamples();
  auto const& desc = all[index];
  CCNode* parent = &layer;
  if (!desc.parentIsLayer)
  {
    parent = new CCMenu();
    parent->id = desc.parent.id;
    parent->size = {WIN.width, WIN.height};
    layer.addChild(parent);
  }
  else
    layer.size = {BOX.width, BOX.height};
  g_layoutNodes.assign(desc.nodes.size(), nullptr);
  for (size_t i = desc.nodes.size(); i-- > 0;)
  {
    auto node = make(desc.nodes[i].className);
    node->id = desc.nodes[i].id;
    parent->addChild(node);
    g_layoutNodes[i] = node;
  }
}

void checkLayoutSample(CCNode& layer, int index)
{
  auto const all = layoutSamples();
  auto const& desc = all[index];
  auto const& fit = desc.fit;
  CCNode* parent = desc.parentIsLayer ? &layer : layer.getChildByIDRecursive(desc.parent.id);
  CHECK(parent != nullptr);
  if (!parent)
    return;
  auto container = parent->getChildByID(fit.row ? "mod.id/row" : "mod.id/column");
  std::printf("layout sample %d: %s\n", index, container ? "made" : "not made");
  CHECK(container != nullptr);
  if (!container)
    return;
  CHECK((dynamic_cast<CCMenu*>(container) != nullptr) == desc.menu);
  CHECK(!container->ignoresAnchor && close(container->anchor.x, 0.5f) && close(container->anchor.y, 0.5f));
  CHECK(close(container->size.width, fit.bounds.width) && close(container->size.height, fit.bounds.height));
  // In the middle of the box of them all, in the parent's space
  CHECK(close(container->position.x, fit.bounds.midX()) && close(container->position.y, fit.bounds.midY()));
  // Every node moved into it, in the order along the axis, held as before
  CHECK(container->children.size() == fit.order.size());
  for (size_t i = 0; i < fit.order.size() && i < container->children.size(); i++)
  {
    CHECK(container->children[i] == g_layoutNodes[fit.order[i]]);
    CHECK(container->children[i]->references == 1);
  }
  auto layout = dynamic_cast<AxisLayout*>(container->layout);
  CHECK(layout != nullptr);
  if (!layout)
    return;
  CHECK(layout->row == fit.row && close(layout->gap, fit.gap) && !layout->autoScale);
  CHECK(layout->reverse == !fit.row);
  auto cross = fit.cross == rowfit::Align::Start ? AxisAlignment::Start
               : fit.cross == rowfit::Align::End ? AxisAlignment::End
                                                  : AxisAlignment::Center;
  CHECK(layout->crossLineAlignment == cross);
  CHECK(container->layoutUpdates == 1);
}

#include "samples.inc"

#include "hooks.inc"

// The hooks called as the game would, through the base class
void runHooks()
{
  {
    Hooked_MenuLayer layer;
    buildScene(layer);
    CHECK(static_cast<MenuLayer&>(layer).init());
    checkSample(0);
    checkSample(1);
  }
  {
    Hooked_PauseLayer layer;
    buildScene(layer);
    static_cast<PauseLayer&>(layer).customSetup();
    checkSample(2);
  }
  {
    // In K&R style
    Hooked_GJGarageLayer layer;
    buildScene(layer);
    CHECK(static_cast<GJGarageLayer&>(layer).init());
    checkSample(3);
  }
  {
    Hooked_LevelSelectLayer layer;
    buildScene(layer);
    CHECK(static_cast<LevelSelectLayer&>(layer).init(3));
    checkSample(4);
  }
}

int main()
{
  runSamples();
  runLayoutSamples();
  runHooks();
  return finish("compiled code");
}
