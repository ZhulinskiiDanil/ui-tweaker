#include "Editor.hpp"

#include "Inspect.hpp"
#include "Retained.hpp"
#include "TreePanel.hpp"
#include "core/Aspect.hpp"
#include "core/Codegen.hpp"
#include "core/History.hpp"
#include "core/Snap.hpp"
#include "core/Text.hpp"
#include "core/Tree.hpp"

#include <Geode/ui/Notification.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <Geode/utils/Keyboard.hpp>

#include <chrono>
#include <cmath>
#include <functional>

using namespace geode::prelude;

namespace
{
  // ! --- State --- !

  enum Kind
  {
    Drag = 1,
    Wheel = 2,
    Nudge = 3,
  };

  // Ahead of every other listener, so the game and other mods don't see what the edit mode takes
  constexpr int PRIORITY = -100000;
  // How far the mouse goes before a press is a drag and not a click, in points
  constexpr float DRAG_THRESHOLD = 2.f;
  // Steps of one kind on one node this close together are one step to undo
  constexpr double WHEEL_MERGE = 0.6;
  constexpr double NUDGE_MERGE = 1.0;
  // How close an edge or a center gets before it snaps, in points
  constexpr float SNAP_DISTANCE = 3.f;

  constexpr float FONT_SCALE = 0.42f;
  constexpr float MARGIN = 4.f;
  constexpr float PANEL_WIDTH = 190.f;
  constexpr size_t PANEL_COLUMNS = 52;

  ccColor4F const HOVER = {0.35f, 0.8f, 1.f, 1.f};
  // CCDrawNode blends premultiplied colors (GL_ONE, GL_ONE_MINUS_SRC_ALPHA): a fill's color is times its alpha
  ccColor4F const HOVER_FILL = {0.35f * 0.07f, 0.8f * 0.07f, 0.07f, 0.07f};
  ccColor4F const SELECTED = {1.f, 0.6f, 0.15f, 1.f};
  ccColor4F const SELECTED_FILL = {0.1f, 0.06f, 0.015f, 0.1f};
  ccColor4F const GUIDE = {1.f, 0.3f, 0.75f, 1.f};
  ccColor3B const GOLD = {255, 210, 90};
  ccColor3B const GREY = {170, 170, 185};

  using NodeState = inspect::NodeState;

  class Overlay;

  struct State
  {
    bool active = false;
    Retained<CCNode> selected;
    history::History<Retained<CCNode>, NodeState> history{200};
    // Every edited node's state before its first edit, for the code of all of them
    std::vector<std::pair<Retained<CCNode>, NodeState>> originals;
    // The scene the history is about: a new one forgets the old one's nodes
    CCScene* scene = nullptr;

    bool pressed = false;
    bool dragging = false;
    bool leftDown = false;
    CCPoint pressPoint;
    NodeState dragStart;
    geom::Affine parentToWorld;

    bool panelDragging = false;
    CCPoint panelGrab;

    std::optional<CCPoint> pinned;
    // What a drag snaps to, gathered when it starts, and the lines it's on now
    std::vector<snap::Guide> guides;
    std::vector<snap::Guide> snapLines;
    std::optional<bool> snapOverride;
    // A: the other screens' shapes over this one
    bool aspectPreview = false;
    // Tab goes through what's under the mouse, deeper each time
    size_t cycle = 0;
    CCPoint cyclePoint;

    std::optional<ListenerHandle> mouse;
    std::optional<ListenerHandle> move;
    std::optional<ListenerHandle> scroll;
    std::optional<ListenerHandle> keys;
    Overlay* overlay = nullptr;
  };

  // Never destroyed: its nodes would be released after Cocos is gone, at the game's exit
  State& state()
  {
    static auto instance = new State();
    return *instance;
  }

  double now()
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  CCScene* runningScene()
  {
    return CCDirector::get()->getRunningScene();
  }

  CCPoint mousePosition()
  {
    auto& s = state();
    return s.pinned ? *s.pinned : getMousePos();
  }

  std::string number(float value)
  {
    auto text = fmt::format("{:.2f}", value);
    while (text.back() == '0')
      text.pop_back();
    if (text.back() == '.')
      text.pop_back();
    return text == "-0" ? "0" : text;
  }

  NodeState const* originalOf(CCNode* node)
  {
    for (auto const& [target, before] : state().originals)
      if (target == node)
        return &before;
    return nullptr;
  }

  void record(CCNode* node, NodeState const& before, Kind kind, double merge)
  {
    auto& s = state();
    auto after = inspect::stateOf(node);
    if (after == before)
      return;
    if (!originalOf(node))
      s.originals.emplace_back(Retained<CCNode>(node), before);
    s.history.record({Retained<CCNode>(node), before, after, kind, now()}, merge);
    static char const* const verbs[] = {"", "Dragged", "Scaled", "Nudged"};
    log::info("{} {}: position ({}, {}), scale {} x {}", verbs[kind], inspect::name(node), number(after.position.x),
              number(after.position.y), number(after.scaleX), number(after.scaleY));
  }

  // Forgets the nodes of a scene that's gone, so they aren't kept alive by the history
  void forgetOtherScenes()
  {
    auto& s = state();
    auto scene = runningScene();
    if (scene == s.scene)
      return;
    s.scene = scene;
    auto gone = [scene](Retained<CCNode> const& node) { return !inspect::isIn(node.get(), scene); };
    s.history.forget(gone);
    std::erase_if(s.originals, [&](auto const& entry) { return gone(entry.first); });
    if (s.selected && gone(s.selected))
      s.selected.reset();
  }

  bool hasModifier(KeyboardModifier modifiers, uint8_t which)
  {
    return (modifiers.value & which) != 0;
  }

  bool isModifier(enumKeyCodes key)
  {
    switch (key)
    {
    case KEY_Shift:
    case KEY_Control:
    case KEY_Alt:
    case KEY_LeftShift:
    case KEY_RightShift:
    case KEY_LeftControl:
    case KEY_RightControl:
    case KEY_LeftMenu:
    case KEY_RightMenu:
      return true;
    default:
      return false;
    }
  }

  // The edit mode's own key: left to Geode, which calls toggle
  bool isToggleKey(KeyboardInputData const& data)
  {
    auto setting = std::dynamic_pointer_cast<KeybindSettingV3>(Mod::get()->getSetting("toggle-key"));
    if (!setting)
      return false;
    for (auto const& bind : setting->getValue())
      if (bind.key == data.key && bind.modifiers == data.modifiers)
        return true;
    return false;
  }

  // The screen's edges and center, the parent's, and the siblings': what a drag of the node snaps to
  std::vector<snap::Guide> guidesFor(CCNode* node)
  {
    std::vector<snap::Guide> guides;
    auto win = CCDirector::get()->getWinSize();
    snap::addBox(guides, {0, 0, win.width, win.height});
    auto parent = node->getParent();
    if (!parent)
      return guides;
    auto size = parent->getContentSize();
    if (size.width > 0 && size.height > 0 && parent != runningScene())
      snap::addBox(guides, inspect::worldQuad(parent).bounds());
    for (auto sibling : CCArrayExt<CCNode*>(parent->getChildren()))
    {
      auto box = sibling->getContentSize();
      if (sibling == node || !sibling->isVisible() || box.width <= 0 || box.height <= 0)
        continue;
      snap::addBox(guides, inspect::worldQuad(sibling).bounds());
    }
    return guides;
  }

  // ! --- Overlay --- !

  struct Button
  {
    CCLayerColor* background = nullptr;
    CCLabelBMFont* label = nullptr;
    std::function<void()> action;
  };

  CCLabelBMFont* makeLabel(CCNode* parent, ccColor3B color = {255, 255, 255})
  {
    auto label = CCLabelBMFont::create("", "chatFont.fnt");
    label->setScale(FONT_SCALE);
    label->setAnchorPoint({0, 1});
    label->setColor(color);
    parent->addChild(label);
    return label;
  }

  // Drawn over every scene: the boxes, the name by the mouse, the panel of the selected node
  class Overlay : public CCNode
  {
  public:
    static Overlay* create()
    {
      auto node = new Overlay();
      if (node->init())
      {
        node->autorelease();
        return node;
      }
      delete node;
      return nullptr;
    }

    bool init() override
    {
      if (!CCNode::init())
        return false;
      setID("overlay"_spr);
      auto win = CCDirector::get()->getWinSize();

      m_draw = CCDrawNode::create();
      addChild(m_draw);

      m_banner = CCLayerColor::create({0, 0, 0, 150}, 10, 10);
      m_bannerText = makeLabel(m_banner, GOLD);
      addChild(m_banner);

      m_tag = CCLayerColor::create({0, 0, 0, 190}, 10, 10);
      m_tagText = makeLabel(m_tag);
      addChild(m_tag);

      m_panel = CCLayerColor::create({15, 15, 25, 215}, PANEL_WIDTH, 10);
      m_title = makeLabel(m_panel, GOLD);
      m_body = makeLabel(m_panel);
      m_warning = makeLabel(m_panel, {255, 200, 90});
      addChild(m_panel);
      m_panelTopLeft = ccp(MARGIN, win.height - 22);

      addButton("Copy", [] { editor::copySelected(); });
      addButton("Copy all", [] { editor::copyAll(); });
      addButton("Undo", [] { editor::undo(); });
      addButton("Redo", [] { editor::redo(); });
      addButton("Leave", [] { Loader::get()->queueInMainThread([] { editor::setActive(false); }); });

      for (auto const& shape : aspect::shapes())
      {
        auto label = makeLabel(this);
        label->setString(shape.name.c_str());
        label->setAnchorPoint({0, 0});
        m_shapeLabels.push_back(label);
      }

      // At the right, under the banner, hidden until T
      m_tree = TreePanel::create();
      m_tree->setPosition({win.width - m_tree->getContentWidth() - MARGIN,
                           win.height - 26 - m_tree->getContentHeight()});
      m_tree->setVisible(false);
      addChild(m_tree);

      setBanner();
      scheduleUpdate();
      return true;
    }

    void update(float) override;

    // ! --- The panel's parts, in this node's space, which is the screen's --- !

    bool panelContains(CCPoint point)
    {
      return m_panel->isVisible() && m_panel->boundingBox().containsPoint(convertToNodeSpace(point));
    }

    bool titleContains(CCPoint point)
    {
      auto local = convertToNodeSpace(point);
      auto box = m_panel->boundingBox();
      return m_panel->isVisible() && box.containsPoint(local) && local.y >= box.getMaxY() - 12;
    }

    Button const* buttonAt(CCPoint point)
    {
      if (!m_panel->isVisible())
        return nullptr;
      auto local = m_panel->convertToNodeSpace(point);
      for (auto const& button : m_buttons)
        if (button.background->boundingBox().containsPoint(local))
          return &button;
      return nullptr;
    }

    void movePanel(CCPoint topLeft)
    {
      auto win = CCDirector::get()->getWinSize();
      auto size = m_panel->getContentSize();
      topLeft.x = std::clamp(topLeft.x, 0.f, std::max(0.f, win.width - size.width));
      topLeft.y = std::clamp(topLeft.y, std::min(size.height, win.height), win.height);
      m_panelTopLeft = topLeft;
      m_panel->setPosition(topLeft - ccp(0, size.height));
    }

    CCPoint panelTopLeft() const { return m_panelTopLeft; }

    void togglePanel() { m_panel->setVisible(!m_panel->isVisible()); }

    TreePanel* tree() const { return m_tree; }

  private:
    CCDrawNode* m_draw = nullptr;
    CCLayerColor* m_banner = nullptr;
    CCLabelBMFont* m_bannerText = nullptr;
    CCLayerColor* m_tag = nullptr;
    CCLabelBMFont* m_tagText = nullptr;
    CCLayerColor* m_panel = nullptr;
    CCLabelBMFont* m_title = nullptr;
    CCLabelBMFont* m_body = nullptr;
    CCLabelBMFont* m_warning = nullptr;
    std::vector<Button> m_buttons;
    std::vector<CCLabelBMFont*> m_shapeLabels;
    TreePanel* m_tree = nullptr;
    CCPoint m_panelTopLeft;
    std::string m_panelKey;

    void addButton(char const* text, std::function<void()> action)
    {
      Button button;
      button.background = CCLayerColor::create({60, 70, 100, 230}, 10, 10);
      button.label = CCLabelBMFont::create(text, "chatFont.fnt");
      button.label->setScale(FONT_SCALE);
      button.label->setAnchorPoint({0.5f, 0.5f});
      button.background->addChild(button.label);
      button.action = std::move(action);
      m_panel->addChild(button.background);
      m_buttons.push_back(std::move(button));
    }

    void setBanner()
    {
      auto setting = std::dynamic_pointer_cast<KeybindSettingV3>(Mod::get()->getSetting("toggle-key"));
      std::string key = setting && !setting->getValue().empty() ? setting->getValue()[0].toString() : "F8";
      m_bannerText->setString(
          fmt::format("UI Tweaker: edit mode ({} leaves)\nDrag: move   Wheel: scale (Shift: fine)   Arrows: 1 px "
                      "(Shift: 10)   Ctrl+Z / Ctrl+Y: undo / redo\nCtrl+C: copy code   Ctrl+Shift+C: copy all   "
                      "Tab: next under the mouse   Alt+click: inside   PgUp: parent   A: other screens   T: tree   H: panel",
                      key)
              .c_str());
      auto win = CCDirector::get()->getWinSize();
      auto size = m_bannerText->getScaledContentSize();
      m_banner->setContentSize(size + ccp(MARGIN * 2, MARGIN));
      m_bannerText->setPosition({MARGIN, size.height + MARGIN / 2});
      m_banner->setPosition({(win.width - m_banner->getContentWidth()) / 2, win.height - m_banner->getContentHeight()});
    }

    void drawQuad(geom::Quad const& quad, ccColor4F fill, float border, ccColor4F color)
    {
      auto bounds = quad.bounds();
      if (bounds.width < 0.01f && bounds.height < 0.01f)
        return;
      CCPoint points[4];
      for (size_t i = 0; i < 4; i++)
        points[i] = convertToNodeSpace({quad.corners[i].x, quad.corners[i].y});
      m_draw->drawPolygon(points, 4, fill, border, color);
    }

    void showTag(CCNode* node)
    {
      if (!node)
      {
        m_tag->setVisible(false);
        return;
      }
      m_tag->setVisible(true);
      auto parent = node->getParent();
      auto text = fmt::format("{}\nin {}", inspect::name(node), parent ? inspect::name(parent) : "nothing");
      if (m_tagText->getString() != text)
        m_tagText->setString(text.c_str());
      auto size = m_tagText->getScaledContentSize();
      m_tag->setContentSize(size + ccp(MARGIN * 2, MARGIN));
      m_tagText->setPosition({MARGIN, size.height + MARGIN / 2});

      // Above the box's top left corner, or under it at the top of the screen, inside the screen
      auto win = CCDirector::get()->getWinSize();
      auto bounds = inspect::worldQuad(node).bounds();
      auto tag = m_tag->getContentSize();
      float x = std::clamp(bounds.minX(), 0.f, std::max(0.f, win.width - tag.width));
      float y = bounds.maxY() + 2;
      if (y + tag.height > win.height)
        y = bounds.minY() - tag.height - 2;
      y = std::clamp(y, 0.f, std::max(0.f, win.height - tag.height));
      m_tag->setPosition(convertToNodeSpace({x, y}));
    }

    void showPanel(CCNode* node)
    {
      auto& s = state();
      std::string title, body, warning;
      if (node)
      {
        auto info = inspect::stateOf(node);
        auto parent = node->getParent();
        auto world = parent ? parent->convertToWorldSpace(info.position) : info.position;
        auto size = node->getContentSize();
        auto desc = inspect::describe(node, originalOf(node));
        title = text::fit(inspect::name(node), PANEL_COLUMNS);
        body = fmt::format("position {}, {}   (screen {}, {})\n"
                           "scale {}{}   rotation {}\n"
                           "anchor {}, {}{}   size {} x {}\n"
                           "z-order {}   parent {}",
                           number(info.position.x), number(info.position.y), number(world.x), number(world.y),
                           number(info.scaleX),
                           info.scaleX == info.scaleY ? "" : fmt::format(" x {}", number(info.scaleY)),
                           number(node->getRotation()), number(info.anchor.x), number(info.anchor.y),
                           node->isIgnoreAnchorPointForPosition() ? " (ignored)" : "", number(size.width),
                           number(size.height), info.zOrder,
                           text::fit(parent ? inspect::name(parent) : "none", 26));
        // The layout's warning says what the code does about it, the other reasons come after it
        auto facts = inspect::factsOf(node);
        std::vector<std::string> reasons;
        if (auto layout = codegen::layoutWarning(desc); !layout.empty())
        {
          reasons.push_back(layout);
          facts.layoutMovesIt = false;
        }
        for (auto& reason : explain::reasons(facts))
          reasons.push_back(std::move(reason));
        auto win = CCDirector::get()->getWinSize();
        if (auto off = aspect::offScreenOn(inspect::worldQuad(node).bounds(), {win.width, win.height}); !off.empty())
          reasons.push_back("Off the screen on " + off + " (A shows them)");
        for (auto const& reason : reasons)
          for (auto const& line : text::wrap("! " + reason, PANEL_COLUMNS))
            warning += line + "\n";
      }
      else
      {
        title = "Nothing selected";
        body = "Click a node to select it, drag to move it";
      }
      auto edited = editor::editedNodes().size();
      auto key = title + body + warning + std::to_string(edited) + (s.history.canUndo() ? "u" : "") +
                 (s.history.canRedo() ? "r" : "");
      if (key == m_panelKey)
        return;
      m_panelKey = key;

      m_title->setString(title.c_str());
      m_body->setString(body.c_str());
      if (!warning.empty())
        warning.pop_back();
      m_warning->setString(warning.c_str());
      m_warning->setVisible(!warning.empty());
      m_buttons[1].label->setString(fmt::format("Copy all ({})", edited).c_str());

      // From the top down
      float y = -MARGIN / 2;
      auto place = [&](CCLabelBMFont* label)
      {
        label->setPosition({MARGIN, y});
        y -= label->getScaledContentHeight() + 1;
      };
      place(m_title);
      place(m_body);
      if (!warning.empty())
        place(m_warning);
      y -= 2;
      float x = MARGIN;
      float buttonHeight = 11;
      for (auto& button : m_buttons)
      {
        auto width = button.label->getScaledContentWidth() + 8;
        button.background->setContentSize({width, buttonHeight});
        button.background->setPosition({x, y - buttonHeight});
        button.label->setPosition({width / 2, buttonHeight / 2});
        x += width + 3;
      }
      // Undo and redo greyed out when there's nothing to do
      m_buttons[2].label->setColor(s.history.canUndo() ? ccColor3B{255, 255, 255} : GREY);
      m_buttons[3].label->setColor(s.history.canRedo() ? ccColor3B{255, 255, 255} : GREY);
      m_buttons[0].label->setColor(node ? ccColor3B{255, 255, 255} : GREY);
      y -= buttonHeight + MARGIN;

      // The labels were placed from the top: move them all up by the height
      float height = -y;
      m_panel->setContentSize({PANEL_WIDTH, height});
      for (auto child : CCArrayExt<CCNode*>(m_panel->getChildren()))
        child->setPositionY(child->getPositionY() + height);
      movePanel(m_panelTopLeft);
    }

    // The narrower and wider screens, centered on this one, the selected node where it would be on each, and a
    // red box around every node with an ID that doesn't fit on one of them
    void drawShapes(CCNode* selected)
    {
      auto win = CCDirector::get()->getWinSize();
      static ccColor4F const colors[] = {
          {1.f, 1.f, 1.f, 0.9f}, {0.4f, 1.f, 0.5f, 0.9f}, {1.f, 0.9f, 0.3f, 0.9f}, {0.4f, 0.8f, 1.f, 0.9f}};
      ccColor4F const none = {0, 0, 0, 0};
      auto const& shapes = aspect::shapes();
      for (size_t i = 0; i < shapes.size(); i++)
      {
        float width = aspect::widthFor(shapes[i], win.height);
        float left = (win.width - width) / 2;
        auto color = colors[i % 4];
        geom::Quad frame{{geom::Vec2{left, 0}, {left + width, 0}, {left + width, win.height}, {left, win.height}}};
        drawQuad(frame, none, 0.5f, color);
        auto label = m_shapeLabels[i];
        label->setVisible(true);
        label->setColor(ccc3(color.r * 255, color.g * 255, color.b * 255));
        label->setPosition(convertToNodeSpace({std::max(left, 0.f) + 2, 12.f + 9.f * i}));
        if (selected)
        {
          auto box = aspect::move(inspect::worldQuad(selected).bounds(), win.width, width);
          geom::Quad ghost{{geom::Vec2{box.minX() + left, box.minY()}, {box.maxX() + left, box.minY()},
                            {box.maxX() + left, box.maxY()}, {box.minX() + left, box.maxY()}}};
          drawQuad(ghost, none, 0.4f, color);
        }
      }
      for (auto node : inspect::shownWithIds(runningScene()))
      {
        auto quad = inspect::worldQuad(node);
        // Backgrounds are made to be cut off
        if (quad.bounds().width >= win.width * 0.9f)
          continue;
        if (!aspect::offScreenOn(quad.bounds(), {win.width, win.height}).empty())
          drawQuad(quad, none, 0.4f, {1.f, 0.25f, 0.25f, 1.f});
      }
    }

  public:
    void redraw(CCNode* hover, CCNode* selected)
    {
      m_draw->clear();
      for (auto label : m_shapeLabels)
        label->setVisible(false);
      if (hover && hover != selected)
        drawQuad(inspect::worldQuad(hover), HOVER_FILL, 0.5f, HOVER);
      if (selected)
      {
        drawQuad(inspect::worldQuad(selected), SELECTED_FILL, 0.8f, SELECTED);
        // The anchor point: where the position is
        if (auto parent = selected->getParent())
          m_draw->drawDot(convertToNodeSpace(parent->convertToWorldSpace(selected->getPosition())), 1.6f, SELECTED);
      }
      for (auto const& line : state().snapLines)
      {
        auto from = line.vertical ? CCPoint{line.at, line.from} : CCPoint{line.from, line.at};
        auto to = line.vertical ? CCPoint{line.at, line.to} : CCPoint{line.to, line.at};
        m_draw->drawSegment(convertToNodeSpace(from), convertToNodeSpace(to), 0.4f, GUIDE);
      }
      if (state().aspectPreview)
        drawShapes(selected);
      showTag(hover ? hover : selected);
      showPanel(selected);
    }
  };

  // ! --- Input --- !

  CCNode* hovered(CCPoint mouse)
  {
    auto& s = state();
    if (s.dragging || s.panelDragging || (s.overlay && s.overlay->panelContains(mouse)))
      return nullptr;
    // Over the tree, the row's node
    if (s.overlay && s.overlay->tree()->contains(mouse))
      return s.overlay->tree()->rowAt(mouse);
    auto hits = inspect::hitsAt(runningScene(), mouse);
    if (hits.empty())
      return nullptr;
    if (ccpDistance(mouse, s.cyclePoint) > 1)
      s.cycle = 0;
    return hits[s.cycle % hits.size()];
  }

  ListenerResult onMouse(MouseInputData& data)
  {
    auto& s = state();
    s.pinned.reset();
    auto mouse = getMousePos();
    using enum MouseInputData::Action;
    using enum MouseInputData::Button;

    if (data.button == Left && data.action == Press)
    {
      s.leftDown = true;
      auto tree = s.overlay ? s.overlay->tree() : nullptr;
      if (tree)
        tree->setTyping(tree->searchAt(mouse));
      if (tree && tree->contains(mouse))
      {
        if (auto node = tree->rowAt(mouse))
          editor::select(node);
      }
      else if (auto button = s.overlay ? s.overlay->buttonAt(mouse) : nullptr)
        button->action();
      else if (s.overlay && s.overlay->titleContains(mouse))
      {
        s.panelDragging = true;
        s.panelGrab = mouse - s.overlay->panelTopLeft();
      }
      else if (!s.overlay || !s.overlay->panelContains(mouse))
        editor::pressAt(mouse, hasModifier(data.modifiers, KeyboardModifier::Alt));
    }
    else if (data.button == Left && data.action == Release)
    {
      // A button held since before the edit mode: the game saw its press, it gets its release
      if (!s.leftDown)
        return ListenerResult::Propagate;
      s.leftDown = false;
      s.panelDragging = false;
      editor::release();
    }
    else if (data.button == Right && data.action == Press)
      editor::select(nullptr);
    return ListenerResult::Stop;
  }

  ListenerResult onKey(KeyboardInputData& data)
  {
    using enum KeyboardInputData::Action;
    if (data.action == Release || isModifier(data.key) || isToggleKey(data))
      return ListenerResult::Propagate;

    auto& s = state();
    bool shift = hasModifier(data.modifiers, KeyboardModifier::Shift);
    bool control = hasModifier(data.modifiers, KeyboardModifier::Control);
    float step = shift ? 10.f : 1.f;

    // The tree's search takes the keys while it's focused
    auto tree = s.overlay ? s.overlay->tree() : nullptr;
    if (tree && tree->isVisible() && tree->typing())
    {
      if (data.key == KEY_Escape)
        tree->setTyping(false);
      else if (data.key == KEY_Enter)
      {
        if (auto node = tree->firstMatch())
          editor::select(node);
        tree->setTyping(false);
      }
      else if (data.key == KEY_Backspace)
        tree->erase();
      else if (auto c = tree::typed(data.key, shift))
        tree->type(*c);
      return ListenerResult::Stop;
    }

    switch (data.key)
    {
    case KEY_T:
      if (tree)
      {
        tree->setVisible(!tree->isVisible());
        tree->setTyping(false);
      }
      break;
    case KEY_Slash:
      if (tree)
      {
        tree->setVisible(true);
        tree->setTyping(true);
      }
      break;
    case KEY_Left:
      editor::nudge(-step, 0);
      break;
    case KEY_Right:
      editor::nudge(step, 0);
      break;
    case KEY_Up:
      editor::nudge(0, step);
      break;
    case KEY_Down:
      editor::nudge(0, -step);
      break;
    case KEY_Z:
      if (control)
        shift ? editor::redo() : editor::undo();
      break;
    case KEY_Y:
      if (control)
        editor::redo();
      break;
    case KEY_C:
      if (control)
        shift ? editor::copyAll() : editor::copySelected();
      break;
    case KEY_Tab:
      editor::nextUnderMouse();
      break;
    case KEY_PageUp:
      if (s.selected && s.selected->getParent() && s.selected->getParent() != runningScene())
        editor::select(s.selected->getParent());
      break;
    case KEY_PageDown:
      if (s.selected)
        for (auto child : CCArrayExt<CCNode*>(s.selected->getChildren()))
          if (child->getContentWidth() > 0 && child->getContentHeight() > 0)
          {
            editor::select(child);
            break;
          }
      break;
    case KEY_A:
      s.aspectPreview = !s.aspectPreview;
      break;
    case KEY_H:
      if (s.overlay)
        s.overlay->togglePanel();
      break;
    case KEY_Escape:
      if (s.selected)
        editor::select(nullptr);
      else
        Loader::get()->queueInMainThread([] { editor::setActive(false); });
      break;
    default:
      break;
    }
    return ListenerResult::Stop;
  }

  void Overlay::update(float)
  {
    auto& s = state();
    forgetOtherScenes();
    // Removed, or the scene changed
    if (s.selected && !inspect::isIn(s.selected.get(), runningScene()))
    {
      s.selected.reset();
      s.pressed = s.dragging = false;
    }

    auto mouse = mousePosition();
    if (s.panelDragging)
      movePanel(mouse - s.panelGrab);
    else if (s.pressed && !s.pinned)
      editor::dragTo(mouse);
    auto hover = hovered(mouse);
    if (m_tree->isVisible())
      m_tree->refresh(runningScene(), s.selected.get(), hover);
    else
      m_tree->forget();
    redraw(hover, s.selected.get());
  }
} // namespace

// ! --- On and off --- !

bool editor::active()
{
  return state().active;
}

void editor::setActive(bool on)
{
  auto& s = state();
  if (on == s.active)
    return;
  s.active = on;

  if (on)
  {
    forgetOtherScenes();
    s.mouse = MouseInputEvent().listen(onMouse, PRIORITY);
    s.move = MouseMoveEvent().listen(
        [](int32_t, int32_t)
        {
          auto& s = state();
          s.pinned.reset();
          // The game gets no moves of a drag it never saw start
          return s.dragging || s.panelDragging ? ListenerResult::Stop : ListenerResult::Propagate;
        },
        PRIORITY);
    s.scroll = ScrollWheelEvent().listen(
        [](double, double y)
        {
          auto& s = state();
          auto tree = s.overlay ? s.overlay->tree() : nullptr;
          if (tree && tree->contains(getMousePos()))
          {
            tree->scroll(static_cast<float>(y));
            return ListenerResult::Stop;
          }
          bool fine = CCKeyboardDispatcher::get()->getShiftKeyPressed();
          editor::wheel(static_cast<float>(y), fine);
          return ListenerResult::Stop;
        },
        PRIORITY);
    s.keys = KeyboardInputEvent().listen(onKey, PRIORITY);

    auto overlay = Overlay::create();
    OverlayManager::get()->addChild(overlay, 10000);
    s.overlay = overlay;
    log::info("Edit mode on");
  }
  else
  {
    release();
    s.mouse.reset();
    s.move.reset();
    s.scroll.reset();
    s.keys.reset();
    s.pressed = s.dragging = s.panelDragging = s.leftDown = false;
    s.pinned.reset();
    s.selected.reset();
    if (s.overlay)
    {
      s.overlay->removeFromParent();
      s.overlay = nullptr;
    }
    log::info("Edit mode off");
  }
}

void editor::toggle()
{
  setActive(!active());
}

// ! --- Editing --- !

CCNode* editor::pressAt(CCPoint world, bool deep)
{
  auto& s = state();
  auto hits = inspect::hitsAt(runningScene(), world, deep);
  if (ccpDistance(world, s.cyclePoint) > 1)
    s.cycle = 0;
  CCNode* node = hits.empty() ? nullptr : hits[s.cycle % hits.size()];
  grab(node, world);
  return node;
}

void editor::grab(CCNode* node, CCPoint world)
{
  auto& s = state();
  select(node);
  if (node && node->getParent())
  {
    s.pressed = true;
    s.dragging = false;
    s.pressPoint = world;
    s.dragStart = inspect::stateOf(node);
    s.parentToWorld = inspect::toGeom(node->getParent()->nodeToWorldTransform());
    s.guides = guidesFor(node);
  }
}

void editor::dragTo(CCPoint world)
{
  auto& s = state();
  if (!s.pressed || !s.selected)
    return;
  auto delta = world - s.pressPoint;
  if (!s.dragging && ccpLength(delta) < DRAG_THRESHOLD)
    return;
  s.dragging = true;
  auto local = geom::localDelta(s.parentToWorld, {delta.x, delta.y});
  auto position = s.dragStart.position + ccp(local.x, local.y);
  s.selected->setPosition(position);

  // Ctrl held: no snapping, as in Figma
  s.snapLines.clear();
  bool snapping = s.snapOverride.value_or(Mod::get()->getSettingValue<bool>("snap"));
  if (!snapping || (!s.snapOverride && CCKeyboardDispatcher::get()->getControlKeyPressed()))
    return;
  auto bounds = inspect::worldQuad(s.selected.get()).bounds();
  auto result = snap::snap(bounds, s.guides, SNAP_DISTANCE);
  if (result.correction.x == 0 && result.correction.y == 0)
    return;
  auto fix = geom::localDelta(s.parentToWorld, result.correction);
  s.selected->setPosition(position + ccp(fix.x, fix.y));
  s.snapLines = std::move(result.lines);
}

void editor::setSnapping(std::optional<bool> on)
{
  state().snapOverride = on;
}

void editor::release()
{
  auto& s = state();
  s.snapLines.clear();
  if (s.dragging && s.selected)
    record(s.selected.get(), s.dragStart, Drag, 0);
  s.pressed = false;
  s.dragging = false;
}

void editor::wheel(float notches, bool fine)
{
  auto& s = state();
  if (!s.selected || notches == 0)
    return;
  auto node = s.selected.get();
  auto before = inspect::stateOf(node);
  auto scaled = [&](float value)
  {
    if (value == 0)
      return value;
    float sign = value < 0 ? -1.f : 1.f;
    float size = std::fabs(value);
    float next = fine ? size + 0.01f * notches : size * std::pow(1.1f, notches);
    next = std::round(next * 100) / 100;
    // Always a step, however small the node is
    if (next == size)
      next = size + (notches > 0 ? 0.01f : -0.01f);
    return sign * std::max(next, 0.01f);
  };
  node->setScaleX(scaled(before.scaleX));
  node->setScaleY(scaled(before.scaleY));
  record(node, before, Wheel, WHEEL_MERGE);
}

void editor::nudge(float dx, float dy)
{
  auto& s = state();
  if (!s.selected || s.dragging)
    return;
  auto node = s.selected.get();
  auto before = inspect::stateOf(node);
  node->setPosition(before.position + ccp(dx, dy));
  record(node, before, Nudge, NUDGE_MERGE);
}

bool editor::undo()
{
  auto& s = state();
  release();
  while (auto change = s.history.undo())
  {
    auto node = change->target.get();
    // Removed since: nothing to put back
    if (!inspect::isIn(node, runningScene()))
      continue;
    inspect::apply(node, change->before);
    select(node);
    log::info("Undid a step of {}: position ({}, {}), scale {} x {}", inspect::name(node),
              number(change->before.position.x), number(change->before.position.y), number(change->before.scaleX),
              number(change->before.scaleY));
    return true;
  }
  return false;
}

bool editor::redo()
{
  auto& s = state();
  release();
  while (auto change = s.history.redo())
  {
    auto node = change->target.get();
    if (!inspect::isIn(node, runningScene()))
      continue;
    inspect::apply(node, change->after);
    select(node);
    log::info("Redid a step of {}", inspect::name(node));
    return true;
  }
  return false;
}

void editor::select(CCNode* node)
{
  auto& s = state();
  if (s.selected == node)
    return;
  release();
  s.selected = Retained<CCNode>(node);
  if (node)
    log::info("Selected {}", inspect::name(node));
}

void editor::nextUnderMouse()
{
  auto& s = state();
  auto mouse = mousePosition();
  if (ccpDistance(mouse, s.cyclePoint) > 1)
    s.cycle = 0;
  s.cycle++;
  s.cyclePoint = mouse;
}

CCNode* editor::selected()
{
  return state().selected.get();
}

size_t editor::historySize()
{
  return state().history.changes().size();
}

// ! --- Code --- !

std::string editor::codeFor(CCNode* node)
{
  if (!node)
    return "";
  return codegen::generate(inspect::describe(node, originalOf(node)));
}

std::vector<CCNode*> editor::editedNodes()
{
  // The nodes with steps not undone, in the order they were first edited. Not "differs from before": a layout
  // that moves a node by itself would make an untouched node look edited
  std::vector<CCNode*> nodes;
  auto& s = state();
  auto scene = runningScene();
  for (auto const& [node, before] : s.originals)
  {
    bool stepped = std::any_of(s.history.changes().begin(), s.history.changes().end(),
                               [&](auto const& change) { return change.target == node; });
    if (stepped && inspect::isIn(node.get(), scene))
      nodes.push_back(node.get());
  }
  return nodes;
}

std::string editor::codeForAll()
{
  std::vector<codegen::NodeDesc> nodes;
  for (auto node : editedNodes())
    nodes.push_back(inspect::describe(node, originalOf(node)));
  return codegen::generateAll(nodes);
}

void editor::copySelected()
{
  auto node = selected();
  if (!node)
  {
    Notification::create("Select a node first", NotificationIcon::Warning, 1.5f)->show();
    return;
  }
  auto code = codeFor(node);
  clipboard::write(code);
  log::info("Code for {}:\n{}", inspect::name(node), code);
  Notification::create(fmt::format("Copied the code for {}", inspect::name(node)), NotificationIcon::Success, 1.5f)
      ->show();
}

void editor::copyAll()
{
  auto nodes = editedNodes();
  if (nodes.empty())
  {
    Notification::create("Nothing is edited in this scene", NotificationIcon::Warning, 1.5f)->show();
    return;
  }
  auto code = codeForAll();
  clipboard::write(code);
  log::info("Code for {} edited nodes:\n{}", nodes.size(), code);
  Notification::create(fmt::format("Copied the code for {} nodes", nodes.size()), NotificationIcon::Success, 1.5f)
      ->show();
}

// ! --- Self test --- !

void editor::setAspectPreview(bool on)
{
  state().aspectPreview = on;
}

void editor::pinMouse(std::optional<CCPoint> world)
{
  state().pinned = world;
}

bool editor::swallowsClick()
{
  using enum MouseInputData::Action;
  MouseInputData press(MouseInputData::Button::Middle, Press, 0, KeyboardModifier::None);
  MouseInputData release(MouseInputData::Button::Middle, Release, 0, KeyboardModifier::None);
  bool stopped = MouseInputEvent().send(press) == ListenerResult::Stop;
  MouseInputEvent().send(release);
  return stopped;
}
