#include "Editor.hpp"

#include "Inspect.hpp"
#include "SelfTest.hpp"
#include "LoggerLink.hpp"
#include "Retained.hpp"
#include "TreePanel.hpp"
#include "Ui.hpp"
#include "core/Aspect.hpp"
#include "core/Codegen.hpp"
#include "core/Fields.hpp"
#include "core/History.hpp"
#include "core/Hooks.hpp"
#include "core/Grid.hpp"
#include "core/Measure.hpp"
#include "core/Session.hpp"
#include "core/Snap.hpp"
#include "core/Text.hpp"
#include "core/Tree.hpp"

#include <Geode/binding/EndLevelLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <Geode/utils/Keyboard.hpp>

#include <algorithm>
#include <chrono>

#include <Windows.h>
#include <cmath>
#include <functional>

using namespace geode::prelude;

namespace
{
  // Further down: the kept edits on disk, and put back on a layer made again
  void saveSession(bool force);
  void putBack();

  // ! --- State --- !

  enum Kind
  {
    Drag = 1,
    Wheel = 2,
    Nudge = 3,
    Reanchor = 4,
    Order = 5,
    Typed = 6,
    Resize = 7,
    Reset = 8,
    Visibility = 9,
    Align = 10,
    PutBack = 11,
    Turn = 12,
  };

  // Ahead of every other listener, so the game and other mods don't see what the edit mode takes
  constexpr int PRIORITY = -100000;
  // How far the mouse goes before a press is a drag and not a click, in points
  constexpr float DRAG_THRESHOLD = 2.f;
  // Steps of one kind on one node this close together are one step to undo
  constexpr double WHEEL_MERGE = 0.6;
  constexpr double NUDGE_MERGE = 1.0;
  // The mouse standing still: the scene is looked through for the hover this often, in frames
  constexpr int HOVER_FRAMES = 10;
  // How close an edge or a center gets before it snaps, in points
  constexpr float SNAP_DISTANCE = 3.f;

  constexpr float FONT_SCALE = 0.42f;
  constexpr float MARGIN = 4.f;
  constexpr float PANEL_WIDTH = 190.f;
  constexpr size_t PANEL_COLUMNS = 52;
  constexpr size_t HELP_COLUMNS = 120;
  constexpr float CODE_SCALE = 0.32f;
  constexpr char NEWLINE = 10;
  constexpr size_t CODE_COLUMNS = 66;

  ccColor4F const HOVER = {0.35f, 0.8f, 1.f, 1.f};
  // CCDrawNode blends premultiplied colors (GL_ONE, GL_ONE_MINUS_SRC_ALPHA): a fill's color is times its alpha
  ccColor4F const HOVER_FILL = {0.35f * 0.07f, 0.8f * 0.07f, 0.07f, 0.07f};
  ccColor4F const SELECTED = {1.f, 0.6f, 0.15f, 1.f};
  ccColor4F const SELECTED_FILL = {0.1f, 0.06f, 0.015f, 0.1f};
  ccColor4F const GUIDE = {1.f, 0.3f, 0.75f, 1.f};
  ccColor4F const MEASURE = {1.f, 0.3f, 0.3f, 1.f};
  ccColor3B const GOLD = {255, 210, 90};
  ccColor3B const GREY = {170, 170, 185};

  using NodeState = inspect::NodeState;

  class Overlay;

  struct State
  {
    bool active = false;
    Retained<CCNode> selected;
    // The other selected nodes, with Shift+click
    std::vector<Retained<CCNode>> also;
    // A drag: every selected node, where it started and its parent's transform
    struct Grab
    {
      Retained<CCNode> node;
      NodeState start;
      geom::Affine parentToWorld;
    };
    std::vector<Grab> grabs;
    // Steps of several nodes made together share a number; the last such step, to merge the next one like it
    int groups = 0;
    struct LastGroup
    {
      int id = 0;
      int kind = 0;
      double time = 0;
      std::vector<CCNode*> targets;
    } lastGroup;
    history::History<Retained<CCNode>, NodeState> history{200};
    // Every edited node's state before its first edit, for the code of all of them
    std::vector<std::pair<Retained<CCNode>, NodeState>> originals;
    // The scene the history is about: a new one forgets the old one's nodes
    // The scene the history is about, by the number the edit mode gave it (a new scene can be allocated where an
    // old one was)
    int scene = 0;
    int scenes = 0;

    bool pressed = false;
    bool dragging = false;
    bool leftDown = false;
    CCPoint pressPoint;
    NodeState dragStart;
    geom::Affine parentToWorld;

    bool panelDragging = false;
    CCPoint panelGrab;

    std::optional<CCPoint> pinned;
    // The cursor a level hid or locked, shown and let go while the edit mode is on, put back after
    bool rehideCursor = false;
    bool relockCursor = false;
    // The last hover and where the mouse was for it
    Retained<CCNode> hover;
    CCPoint hoverPoint;
    size_t hoverCycle = 0;
    int hoverFrames = 0;
    // What a drag snaps to, gathered when it starts, and the lines it's on now
    std::vector<snap::Guide> guides;
    std::vector<snap::Guide> snapLines;
    std::optional<bool> snapOverride;
    std::optional<int> gridOverride;
    // The grid's step while a drag uses it, for drawing it
    int gridShown = 0;
    std::optional<bool> putBackOverride;
    // A: the other screens' shapes over this one
    bool aspectPreview = false;
    // C: the selected node's code in the panel
    bool codeView = false;
    // Enter: values being typed for the selected node, and the one that couldn't be read
    std::optional<fields::Fields> typing;

    // A resize handle being dragged: which one, from where, the node's state and its world-to-node turn then
    struct Resizing
    {
      geom::Vec2 handle;
      CCPoint press;
      NodeState start;
      geom::NodeTransform transform;
      geom::Affine worldToNode;
    };
    std::optional<Resizing> resizing;
    // The nodes whose size was set by a resize or typed values: other nodes' sizes are their own (a label's
    // text, a layout growing) and aren't written or put back
    std::vector<CCNode*> sized;
    // The same for visibility: only V's is an edit, not the game hiding a tab's buttons
    std::vector<CCNode*> toggled;

    // The code of every edited node, kept up to date while its scene runs: the scene's nodes can't be described
    // once it's gone. Keyed by node, compared only
    std::vector<std::pair<CCNode*, codegen::NodeDesc>> live;
    std::string liveLayer;
    // Edits of scenes left, by the layer they were in, for "Copy all"
    std::vector<std::pair<std::string, std::vector<codegen::NodeDesc>>> earlier;
    // The kept edits changed since they were written to the save folder
    bool sessionDirty = false;
    double sessionSaved = 0;
    // A scene whose layers get their kept edits put back once it has settled, and from when
    int putBackScene = 0;
    double putBackAt = 0;
    std::string typingError;
    std::vector<std::string> typingStart;
    std::vector<float> typingExact;
    // Alt held: the distances from the selected node to the hovered one
    std::optional<bool> measureOverride;
    std::vector<measure::Segment> measured;
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

  // The scene's main layer: "MenuLayer"
  std::string layerName(CCScene* scene)
  {
    if (!scene)
      return "";
    for (auto child : CCArrayExt<CCNode*>(scene->getChildren()))
      if (typeinfo_cast<CCLayer*>(child))
        return inspect::className(child);
    return inspect::className(scene);
  }

  bool hasSteps(CCNode* node)
  {
    auto const& changes = state().history.changes();
    return std::any_of(changes.begin(), changes.end(), [&](auto const& change) { return change.target == node; });
  }

  // Whether a step not undone turned the node, or changed its opacity: a spinning or fading node isn't edited by
  // the game's actions, and an undone turn isn't one
  bool turnedHere(CCNode* node)
  {
    auto const& changes = state().history.changes();
    return std::any_of(changes.begin(), changes.end(), [&](auto const& change)
                       { return change.target == node && change.before.rotation != change.after.rotation; });
  }

  bool fadedHere(CCNode* node)
  {
    auto const& changes = state().history.changes();
    return std::any_of(changes.begin(), changes.end(), [&](auto const& change)
                       { return change.target == node && change.before.opacity != change.after.opacity; });
  }

  // The code laid out as the setting says
  std::string styled(std::string code)
  {
    if (Mod::get()->getSettingValue<std::string>("code-style") == "K&R, 4 spaces")
      return codegen::toKAndR(code);
    return code;
  }

  bool isSized(CCNode* node)
  {
    auto const& sized = state().sized;
    return std::find(sized.begin(), sized.end(), node) != sized.end();
  }

  // The node for the code: its size only if it was set here
  codegen::NodeDesc describeEdited(CCNode* node)
  {
    auto desc = inspect::describe(node, originalOf(node));
    desc.sizeChanged = desc.sizeChanged && isSized(node);
    auto const& toggled = state().toggled;
    desc.visibleChanged = desc.visibleChanged && std::find(toggled.begin(), toggled.end(), node) != toggled.end();
    desc.rotationChanged = desc.rotationChanged && turnedHere(node);
    desc.opacityChanged = desc.opacityChanged && fadedHere(node);
    return desc;
  }

  // The node's code kept for when its scene is gone
  void snapshot(CCNode* node)
  {
    auto& s = state();
    std::erase_if(s.live, [&](auto const& entry) { return entry.first == node; });
    if (hasSteps(node))
    {
      s.live.emplace_back(node, describeEdited(node));
      s.liveLayer = layerName(runningScene());
    }
  }

  void record(CCNode* node, NodeState const& before, Kind kind, double merge, int group = 0)
  {
    auto& s = state();
    auto after = inspect::stateOf(node);
    if (after == before)
      return;
    if ((kind == Resize || kind == Typed) && after.size != before.size &&
        std::find(s.sized.begin(), s.sized.end(), node) == s.sized.end())
      s.sized.push_back(node);
    if (kind == Visibility && after.visible != before.visible &&
        std::find(s.toggled.begin(), s.toggled.end(), node) == s.toggled.end())
      s.toggled.push_back(node);

    if (!originalOf(node))
      s.originals.emplace_back(Retained<CCNode>(node), before);
    s.history.record({Retained<CCNode>(node), before, after, kind, now(), group}, merge);
    snapshot(node);
    s.sessionDirty = true;
    static char const* const verbs[] = {"", "Dragged", "Scaled", "Nudged", "Re-anchored", "Reordered", "Typed", "Resized", "Reset", "Shown/hidden", "Aligned", "Put back", "Turned"};
    log::info("{} {}: position ({}, {}), scale {} x {}", verbs[kind], inspect::name(node), number(after.position.x),
              number(after.position.y), number(after.scaleX), number(after.scaleY));
  }

  // A number for the scene, kept on it while it lives
  int sceneTag(CCScene* scene)
  {
    if (!scene)
      return 0;
    if (auto tag = typeinfo_cast<CCInteger*>(scene->getUserObject("scene"_spr)))
      return tag->getValue();
    int tag = ++state().scenes;
    scene->setUserObject("scene"_spr, CCInteger::create(tag));
    return tag;
  }

  // The descriptions of nodes into a group, a node with an ID replacing its older code there
  void addTo(std::vector<codegen::NodeDesc>& group, codegen::NodeDesc const& desc)
  {
    std::erase_if(group, [&](auto const& old) { return session::sameNode(old, desc); });
    group.push_back(desc);
  }

  std::vector<codegen::NodeDesc>& groupOf(std::string const& layer)
  {
    auto& s = state();
    auto group = std::find_if(s.earlier.begin(), s.earlier.end(), [&](auto const& g) { return g.first == layer; });
    if (group != s.earlier.end())
      return group->second;
    s.earlier.emplace_back(layer, std::vector<codegen::NodeDesc>{});
    return s.earlier.back().second;
  }

  // Edited nodes' code moved from what's being worked on to what's kept for "Copy all": the nodes the history is
  // about to let go of (which it still holds, so they can be looked at). Undone ones are left out
  template <class Which>
  void fileAway(Which which)
  {
    auto& s = state();
    std::erase_if(s.live,
                  [&](auto const& entry)
                  {
                    if (!which(entry.first))
                      return false;
                    if (hasSteps(entry.first))
                      addTo(groupOf(entry.second.layer.empty() ? s.liveLayer : entry.second.layer), entry.second);
                    return true;
                  });
  }

  // The nodes whose size, visibility, rotation or opacity the edit mode set: only ones still held (every edited
  // node's original state holds it), never a node freed since, which a new one could share an address with
  void pruneTracked()
  {
    auto& s = state();
    auto held = [&](CCNode* node)
    { return std::any_of(s.originals.begin(), s.originals.end(), [&](auto const& entry) { return entry.first.get() == node; }); };
    for (auto list : {&s.sized, &s.toggled})
      std::erase_if(*list, [&](CCNode* node) { return !held(node); });
  }

  // Forgets the nodes of a scene that's gone, so they aren't kept alive by the history
  void forgetOtherScenes()
  {
    auto& s = state();
    auto scene = runningScene();
    auto tag = sceneTag(scene);
    if (tag == s.scene)
      return;
    s.scene = tag;
    // The edits of the scene left, kept by its layer
    fileAway([](CCNode*) { return true; });
    s.sessionDirty = true;
    s.putBackScene = tag;
    s.putBackAt = now() + 0.3;
    auto gone = [scene](Retained<CCNode> const& node) { return !inspect::isIn(node.get(), scene); };
    s.history.forget(gone);
    std::erase_if(s.originals, [&](auto const& entry) { return gone(entry.first); });
    pruneTracked();
    if (s.selected && gone(s.selected))
    {
      s.selected.reset();
      s.typing.reset();
      s.pressed = s.dragging = false;
    }
    s.hover.reset();
  }

  // Characters come to a focused text field through the IME, not as key events: the edit mode takes the focus
  // away so the tree's search and the edit mode's keys don't type into the game
  void unfocusTextFields(CCNode* node)
  {
    if (!node || !CCIMEDispatcher::sharedDispatcher()->hasDelegate())
      return;
    if (auto input = typeinfo_cast<CCTextInputNode*>(node))
      if (input->m_textField)
        input->m_textField->detachWithIME();
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      unfocusTextFields(child);
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

  // The edit mode's own key (or mouse button): left to Geode, which calls toggle
  bool isToggle(enumKeyCodes key, KeyboardModifier modifiers)
  {
    auto setting = std::dynamic_pointer_cast<KeybindSettingV3>(Mod::get()->getSetting("toggle-key"));
    if (!setting)
      return false;
    for (auto const& bind : setting->getValue())
      if (bind.key == key && bind.modifiers == modifiers)
        return true;
    return false;
  }

  bool isToggleKey(KeyboardInputData const& data)
  {
    return isToggle(data.key, data.modifiers);
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

  // The nodes of one step selected together, the first shown in the panel. A selection that has them all stays:
  // a node an alignment didn't move has no change in the step, and is still meant to be selected
  void selectAll(std::vector<CCNode*> const& nodes)
  {
    auto& s = state();
    auto current = editor::selection();
    if (std::all_of(nodes.begin(), nodes.end(),
                    [&](CCNode* node) { return std::find(current.begin(), current.end(), node) != current.end(); }))
      return;
    editor::select(nodes.front());
    for (size_t i = 1; i < nodes.size(); i++)
      s.also.emplace_back(nodes[i]);
  }

  // An edit of every selected node: one step for all of them, merged with the last one like it within `merge`
  // (a turn of the wheel, a run of arrow presses)
  template <class Edit>
  void editAll(Kind kind, double merge, Edit edit)
  {
    auto& s = state();
    auto nodes = editor::selection();
    if (nodes.size() == 1)
    {
      auto before = inspect::stateOf(nodes[0]);
      edit(nodes[0]);
      record(nodes[0], before, kind, merge);
      return;
    }
    auto const& changes = s.history.changes();
    bool again = s.lastGroup.id != 0 && s.lastGroup.kind == kind && now() - s.lastGroup.time <= merge &&
                 s.lastGroup.targets == nodes && !changes.empty() && changes.back().group == s.lastGroup.id;
    int group = again ? s.lastGroup.id : ++s.groups;
    for (auto node : nodes)
    {
      auto before = inspect::stateOf(node);
      edit(node);
      if (again && s.history.amend(group, node, inspect::stateOf(node), now()))
        snapshot(node);
      else
        record(node, before, kind, 0, group);
    }
    s.lastGroup = {group, kind, now(), nodes};
  }

  constexpr float HANDLE_SIZE = 1.6f;

  // Characters, not bytes: Cyrillic is two bytes a letter in UTF-8
  size_t characters(std::string const& text)
  {
    return std::count_if(text.begin(), text.end(), [](char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; });
  }

  // The resize handles of a node on the screen: corners and the middles of the sides, as -1, 0, 1 on each axis.
  // None for a node without a size, or with several selected
  std::vector<std::pair<CCPoint, CCPoint>> handles(CCNode* node)
  {
    std::vector<std::pair<CCPoint, CCPoint>> out;
    if (!node || !state().also.empty())
      return out;
    auto size = node->getContentSize();
    if (size.width <= 0 || size.height <= 0)
      return out;
    // A label's size is its text's, a sprite's its texture's: a new size wouldn't stretch them
    if (typeinfo_cast<CCLabelBMFont*>(node) || typeinfo_cast<CCLabelTTF*>(node) ||
        (typeinfo_cast<CCSprite*>(node) && !typeinfo_cast<CCMenuItem*>(node)))
      return out;
    auto toWorld = inspect::toGeom(node->nodeToWorldTransform());
    // Handles closer together than this would cover the node's middle, where a press drags it
    auto bounds = inspect::worldQuad(node).bounds();
    bool sides = bounds.width >= HANDLE_SIZE * 8 && bounds.height >= HANDLE_SIZE * 8;
    if (bounds.width < HANDLE_SIZE * 4 || bounds.height < HANDLE_SIZE * 4)
      return out;
    for (int x = -1; x <= 1; x++)
      for (int y = -1; y <= 1; y++)
      {
        if ((x == 0 && y == 0) || ((x == 0 || y == 0) && !sides))
          continue;
        auto at = toWorld.apply({(x + 1) / 2.f * size.width, (y + 1) / 2.f * size.height});
        out.emplace_back(CCPoint(x, y), CCPoint(at.x, at.y));
      }
    return out;
  }

  // ! --- Overlay --- !

  struct Button
  {
    CCLayerColor* background = nullptr;
    UiLabel* label = nullptr;
    std::function<void()> action;
  };

  UiLabel* makeLabel(CCNode* parent, ccColor3B color = {255, 255, 255})
  {
    auto label = UiLabel::create(FONT_SCALE);
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
      // The code, bitmap in any language (it's ASCII), smaller: its lines are long
      m_code = UiLabel::create(CODE_SCALE, true);
      m_code->setAnchorPoint({0, 1});
      m_code->setColor({150, 220, 160});
      m_panel->addChild(m_code);
      addChild(m_panel);
      m_panelTopLeft = ccp(MARGIN, win.height);

      auto const& w = ui::words();
      addButton(w.copy, [] { editor::copySelected(); });
      addButton(text::format(w.copyAll, {"0"}), [] { editor::copyAll(); });
      addButton(w.undo, [] { editor::undo(); });
      addButton(w.redo, [] { editor::redo(); });
      addButton(w.leave, [] { Loader::get()->queueInMainThread([] { editor::setActive(false); }); });

      for (auto const& shape : aspect::shapes())
      {
        auto label = makeLabel(this);
        label->setString(shape.name);
        label->setAnchorPoint({0, 0});
        m_shapeLabels.push_back(label);
      }

      for (int i = 0; i < 4; i++)
      {
        auto tag = CCLayerColor::create({200, 50, 50, 230}, 10, 8);
        auto text = makeLabel(tag);
        text->setAnchorPoint({0.5f, 0.5f});
        text->setTag(1);
        tag->setVisible(false);
        addChild(tag);
        m_measureTags.push_back(tag);
      }

      // At the right, under the banner, hidden until T
      m_tree = TreePanel::create();
      m_tree->setPosition({win.width - m_tree->getContentWidth() - MARGIN,
                           win.height - 26 - m_tree->getContentHeight()});
      m_tree->setVisible(Mod::get()->getSavedValue<bool>("tree", false));
      m_help = Mod::get()->getSavedValue<bool>("help", false);
      // Where the user put the panel last time, if they did
      if (Mod::get()->getSavedValue<bool>("panel-placed", false))
      {
        m_panelPlaced = true;
        m_panelTopLeft = ccp(Mod::get()->getSavedValue<float>("panel-x", MARGIN),
                             Mod::get()->getSavedValue<float>("panel-y", win.height));
      }
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

    // Dragged by its title: where it is is the user's, it doesn't move out of the way any more
    void panelMoved() { m_panelPlaced = true; }

    // Right click on the title: the panel goes where it was and moves out of the way again
    void panelUnplaced()
    {
      m_panelPlaced = false;
      movePanel({MARGIN, CCDirector::get()->getWinSize().height});
      setBanner();
      savePrefs();
    }

    // The panel over the selected node goes to the other side of the screen, unless it was put there by hand
    void dodge(CCNode* selected)
    {
      if (m_panelPlaced || !selected || !m_panel->isVisible())
        return;
      auto box = m_panel->boundingBox();
      auto bounds = inspect::worldQuad(selected).bounds();
      auto low = convertToNodeSpace({bounds.minX(), bounds.minY()});
      auto high = convertToNodeSpace({bounds.maxX(), bounds.maxY()});
      CCRect node(low.x, low.y, high.x - low.x, high.y - low.y);
      if (!box.intersectsRect(node))
        return;
      auto win = CCDirector::get()->getWinSize();
      bool left = m_panelTopLeft.x < win.width / 2;
      // The tree is at the right: then the panel goes down instead
      CCPoint target = left && m_tree->isVisible()
                           ? CCPoint{m_panelTopLeft.x, box.size.height + MARGIN}
                           : CCPoint{left ? win.width - box.size.width - MARGIN : MARGIN, m_panelTopLeft.y};
      // A node as big as the screen is under it anywhere: then it stays, rather than jump every frame
      CCRect there(target.x, target.y - box.size.height, box.size.width, box.size.height);
      if (!there.intersectsRect(node))
        movePanel(target);
    }

    void togglePanel() { m_panel->setVisible(!m_panel->isVisible()); }

    // What's open and where the panel is, for the next time
    void savePrefs()
    {
      auto& s = state();
      Mod::get()->setSavedValue("tree", m_tree->isVisible());
      Mod::get()->setSavedValue("help", m_help);
      Mod::get()->setSavedValue("code", s.codeView);
      Mod::get()->setSavedValue("aspect", s.aspectPreview);
      Mod::get()->setSavedValue("panel-placed", m_panelPlaced);
      Mod::get()->setSavedValue("panel-x", m_panelTopLeft.x);
      Mod::get()->setSavedValue("panel-y", m_panelTopLeft.y);
    }

    void toggleHelp()
    {
      m_help = !m_help;
      savePrefs();
      setBanner();
      movePanel(m_panelTopLeft);
    }

    TreePanel* tree() const { return m_tree; }

  private:
    CCDrawNode* m_draw = nullptr;
    CCLayerColor* m_banner = nullptr;
    UiLabel* m_bannerText = nullptr;
    CCLayerColor* m_tag = nullptr;
    UiLabel* m_tagText = nullptr;
    CCLayerColor* m_panel = nullptr;
    UiLabel* m_title = nullptr;
    UiLabel* m_body = nullptr;
    UiLabel* m_warning = nullptr;
    UiLabel* m_code = nullptr;
    std::vector<Button> m_buttons;
    std::vector<UiLabel*> m_shapeLabels;
    std::vector<CCLayerColor*> m_measureTags;
    TreePanel* m_tree = nullptr;
    bool m_panelPlaced = false;
    // The selected node's look, and which node it's of (compared only)
    CCNode* m_lookNode = nullptr;
    std::string m_look;
    bool m_help = false;
    CCPoint m_panelTopLeft;
    std::string m_panelKey;

    void addButton(std::string const& text, std::function<void()> action)
    {
      Button button;
      button.background = CCLayerColor::create({60, 70, 100, 230}, 10, 10);
      button.label = UiLabel::create(FONT_SCALE);
      button.label->setString(text);
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
      auto const& w = ui::words();
      // One line, out of the way of what's at the top of the screen; F1 for every key
      auto title = text::format(w.banner, {key});
      // Every key, wrapped to stay inside the screen
      std::string help = title;
      if (m_help)
      {
        // Whole "key: what" items to a line, never split
        auto all = w.help1 + "   " + w.help2;
        std::string line;
        size_t at = 0;
        while (at < all.size())
        {
          auto end = all.find("   ", at);
          auto item = all.substr(at, end == std::string::npos ? std::string::npos : end - at);
          at = end == std::string::npos ? all.size() : end + 3;
          if (!line.empty() && characters(line) + 3 + characters(item) > HELP_COLUMNS)
          {
            help += "\n" + line;
            line.clear();
          }
          line += (line.empty() ? "" : "   ") + item;
        }
        if (!line.empty())
          help += "\n" + line;
      }
      m_bannerText->setString(m_help ? help : title + "   " + w.helpHint);
      auto win = CCDirector::get()->getWinSize();
      auto size = m_bannerText->getScaledContentSize();
      m_banner->setContentSize(size + ccp(MARGIN * 2, MARGIN));
      m_bannerText->setPosition({MARGIN, size.height + MARGIN / 2});
      m_banner->setPosition({(win.width - m_banner->getContentWidth()) / 2, win.height - m_banner->getContentHeight()});
      // Under the banner, unless dragged lower already
      m_panelTopLeft.y = std::min(m_panelTopLeft.y, win.height - m_banner->getContentHeight() - 2);
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
      auto text = text::format(ui::words().tag, {inspect::name(node), parent ? inspect::name(parent) : ui::words().none});
      m_tagText->setString(text);
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
        auto desc = describeEdited(node);
        title = text::fit(inspect::name(node), PANEL_COLUMNS);
        if (!s.also.empty())
          title = text::fit(text::format(ui::words().selectedCount, {std::to_string(s.also.size() + 1)}) + ": " +
                                inspect::name(node),
                            PANEL_COLUMNS);
        auto const& w = ui::words();
        body = text::format(w.position, {number(info.position.x), number(info.position.y), number(world.x),
                                         number(world.y)}) +
               "\n" +
               text::format(w.scale, {number(info.scaleX),
                                      info.scaleX == info.scaleY ? "" : " x " + number(info.scaleY)}) +
               "   " + text::format(w.rotation, {number(node->getRotation())}) +
               (typeinfo_cast<CCRGBAProtocol*>(node) ? "   " + text::format(w.opacity, {std::to_string(info.opacity)}) : "") +
               "\n" +
               text::format(w.anchor, {number(info.anchor.x), number(info.anchor.y),
                                       node->isIgnoreAnchorPointForPosition() ? w.ignored : ""}) +
               "   " + text::format(w.size, {number(size.width), number(size.height)}) + "\n" +
               text::format(w.zOrder, {std::to_string(info.zOrder),
                                       text::fit(parent ? inspect::name(parent) : w.none, 26)});
        // A label's font and text as they are now; a sprite's frame looked for once a node, it goes through
        // every frame of the cache
        if (typeinfo_cast<CCLabelProtocol*>(node))
          m_look = inspect::lookOf(node);
        else if (node != m_lookNode)
          m_look = inspect::lookOf(node);
        m_lookNode = node;
        if (!m_look.empty())
          body += "\n" + text::format(w.look, {text::fit(m_look, PANEL_COLUMNS - 10)});
        // The layout's warning says what the code does about it, the other reasons come after it
        auto facts = inspect::factsOf(node);
        std::vector<std::string> reasons;
        if (auto layout = codegen::layoutWarning(desc, ui::language()); !layout.empty())
        {
          reasons.push_back(layout);
          facts.layoutMovesIt = false;
        }
        for (auto& reason : explain::reasons(facts, ui::language()))
          reasons.push_back(std::move(reason));
        auto win = CCDirector::get()->getWinSize();
        if (auto off = aspect::offScreenOn(inspect::worldQuad(node).bounds(), {win.width, win.height}); !off.empty())
          reasons.push_back(text::format(ui::words().offScreen, {off}));
        for (auto const& reason : reasons)
          for (auto const& line : text::wrap("! " + reason, PANEL_COLUMNS))
            warning += line + "\n";
      }
      else
      {
        title = ui::words().nothingSelected;
        // A node made later at the same address isn't this one
        m_lookNode = nullptr;
        body = ui::words().clickHint;
      }
      auto edited = editor::editedCount();
      if (node && s.typing)
      {
        auto const& w = ui::words();
        std::string const names[] = {"x", "y", w.scaleField, w.widthField, w.heightField, w.rotationField, w.opacityField};
        std::string line;
        for (size_t i = 0; i < s.typing->values().size(); i++)
        {
          auto name = i < std::size(names) ? names[i] : std::string();
          bool current = i == s.typing->current();
          line += name + (current ? " [" : " ") + s.typing->values()[i] + (current ? "_]" : "") + "   ";
        }
        body += "\n" + line;
        auto hint = s.typingError.empty() ? w.typingHint : text::format(w.typingError, {s.typingError});
        for (auto const& hintLine : text::wrap(hint, PANEL_COLUMNS))
          body += "\n" + hintLine;
      }
      // C: the code Ctrl+C would copy, as it is now
      std::string code;
      if (node && s.codeView)
      {
        auto text = editor::codeFor(node);
        size_t start = 0;
        while (start < text.size())
        {
          auto end = text.find(NEWLINE, start);
          auto line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
          start = end == std::string::npos ? text.size() : end + 1;
          // Indented lines stay indented when they wrap
          auto indent = line.substr(0, std::min(line.find_first_not_of(' '), line.size()));
          bool first = true;
          for (auto const& part : text::wrap(line.substr(indent.size()), CODE_COLUMNS - indent.size()))
          {
            code += indent + (first ? "" : "  ") + part + NEWLINE;
            first = false;
          }
        }
        if (!code.empty())
          code.pop_back();
      }
      auto key = title + body + warning + code + std::to_string(edited) + (s.history.canUndo() ? "u" : "") +
                 (s.history.canRedo() ? "r" : "");
      if (key == m_panelKey)
        return;
      m_panelKey = key;
      logger_link::pin(node ? title + ": " + body.substr(0, body.find('\n')) : "nothing", edited);

      m_title->setString(title);
      m_body->setString(body);
      if (!warning.empty())
        warning.pop_back();
      m_warning->setString(warning);
      m_warning->setVisible(!warning.empty());
      m_code->setString(code);
      m_code->setVisible(!code.empty());
      m_buttons[1].label->setString(text::format(ui::words().copyAll, {std::to_string(edited)}));

      // From the top down
      float y = -MARGIN / 2;
      auto place = [&](UiLabel* label)
      {
        label->setPosition({MARGIN, y});
        y -= label->getScaledContentHeight() + 1;
      };
      place(m_title);
      place(m_body);
      if (!warning.empty())
        place(m_warning);
      if (!code.empty())
        place(m_code);
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

    // The grid a drag snaps the code's numbers to: the screen's lines for a node measured on the screen, its parent's,
    // through the parent's transform, for one measured in its parent. Faint, and none finer than 4 points
    void drawGrid(CCNode* node, float step)
    {
      if (step < 4)
        return;
      auto desc = inspect::describe(node);
      ccColor4F const color = {0.55f * 0.14f, 0.75f * 0.14f, 0.14f, 0.14f};
      auto segment = [&](CCPoint from, CCPoint to) { m_draw->drawSegment(convertToNodeSpace(from), convertToNodeSpace(to), 0.25f, color); };
      if (desc.frame.kind == codegen::FrameKind::Screen)
      {
        auto win = CCDirector::get()->getWinSize();
        for (auto x : grid::lines(win.width, step))
          segment({x, 0}, {x, win.height});
        for (auto y : grid::lines(win.height, step))
          segment({0, y}, {win.width, y});
        return;
      }
      auto parent = node->getParent();
      auto size = parent->getContentSize();
      auto toWorld = [&](float x, float y) { return parent->convertToWorldSpace({x, y}); };
      for (auto x : grid::lines(size.width, step))
        segment(toWorld(x, 0), toWorld(x, size.height));
      for (auto y : grid::lines(size.height, step))
        segment(toWorld(0, y), toWorld(size.width, y));
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
        CCPoint at{std::max(left, 0.f) + 2, 12.f + 9.f * i};
        label->setPosition(convertToNodeSpace(at));
        // On a dark plate: colored text alone is lost on a busy screen (a logo of the same color)
        auto size = label->getScaledContentSize();
        geom::Quad plate{{geom::Vec2{at.x - 1.5f, at.y - 0.5f}, {at.x + size.width + 1.5f, at.y - 0.5f},
                          {at.x + size.width + 1.5f, at.y + size.height + 0.5f}, {at.x - 1.5f, at.y + size.height + 0.5f}}};
        drawQuad(plate, {0.f, 0.f, 0.f, 0.7f}, 0.f, none);
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

    // Lines with their lengths, between the selected node and the hovered one
    void drawMeasures(CCNode* hover, CCNode* selected)
    {
      auto& s = state();
      for (auto tag : m_measureTags)
        tag->setVisible(false);
      s.measured.clear();
      bool on = s.measureOverride.value_or(CCKeyboardDispatcher::get()->getAltKeyPressed());
      if (!on || !hover || !selected || hover == selected)
        return;
      s.measured = measure::between(inspect::worldQuad(selected).bounds(), inspect::worldQuad(hover).bounds());
      for (size_t i = 0; i < s.measured.size() && i < m_measureTags.size(); i++)
      {
        auto const& line = s.measured[i];
        auto from = convertToNodeSpace({line.from.x, line.from.y});
        auto to = convertToNodeSpace({line.to.x, line.to.y});
        m_draw->drawSegment(from, to, 0.35f, MEASURE);
        auto tag = m_measureTags[i];
        auto text = static_cast<UiLabel*>(tag->getChildByTag(1));
        auto value = number(line.length);
        text->setString(value);
        auto size = text->getScaledContentSize();
        tag->setContentSize(size + ccp(4, 1));
        text->setPosition(tag->getContentSize() / 2);
        tag->setPosition((from + to) / 2 - tag->getContentSize() / 2);
        tag->setVisible(true);
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
        // The rest of a selection of several, thinner
        for (auto const& other : state().also)
          drawQuad(inspect::worldQuad(other.get()), SELECTED_FILL, 0.5f, SELECTED);
        // The anchor point: where the position is
        if (auto parent = selected->getParent())
          m_draw->drawDot(convertToNodeSpace(parent->convertToWorldSpace(selected->getPosition())), 1.6f, SELECTED);
        for (auto const& [handle, at] : handles(selected))
        {
          auto point = convertToNodeSpace(at);
          CCPoint square[4] = {point + ccp(-HANDLE_SIZE, -HANDLE_SIZE), point + ccp(HANDLE_SIZE, -HANDLE_SIZE), point + ccp(HANDLE_SIZE, HANDLE_SIZE),
                               point + ccp(-HANDLE_SIZE, HANDLE_SIZE)};
          m_draw->drawPolygon(square, 4, {1, 1, 1, 1}, 0.4f, SELECTED);
        }
      }
      if (state().gridShown > 0 && state().dragging && selected && selected->getParent())
        drawGrid(selected, static_cast<float>(state().gridShown));
      for (auto const& line : state().snapLines)
      {
        auto from = line.vertical ? CCPoint{line.at, line.from} : CCPoint{line.from, line.at};
        auto to = line.vertical ? CCPoint{line.at, line.to} : CCPoint{line.to, line.at};
        m_draw->drawSegment(convertToNodeSpace(from), convertToNodeSpace(to), 0.4f, GUIDE);
      }
      if (state().aspectPreview)
        drawShapes(selected);
      drawMeasures(hover, selected);
      showTag(hover ? hover : selected);
      showPanel(selected);
    }
  };

  // ! --- Input --- !

  // What's under the mouse, held: a node that removes itself this frame stays valid until the frame is drawn
  Retained<CCNode> hovered(CCPoint mouse)
  {
    auto& s = state();
    if (s.dragging || s.panelDragging || (s.overlay && s.overlay->panelContains(mouse)))
      return {};
    // Over the tree, the row's node (checked to be in the scene: the tree doesn't hold its nodes)
    if (s.overlay && s.overlay->tree()->contains(mouse))
      return Retained<CCNode>(s.overlay->tree()->rowAt(mouse, runningScene()));

    // The whole scene is looked through only when the mouse moved or Tab was pressed, else a few times a second
    bool still = ccpDistance(mouse, s.hoverPoint) < 0.5f && s.hoverCycle == s.cycle;
    if (still && ++s.hoverFrames < HOVER_FRAMES && (!s.hover || inspect::isIn(s.hover.get(), runningScene())))
      return s.hover;
    s.hoverFrames = 0;
    s.hoverPoint = mouse;
    s.hoverCycle = s.cycle;
    auto hits = inspect::hitsAt(runningScene(), mouse);
    if (ccpDistance(mouse, s.cyclePoint) > 1)
      s.cycle = 0;
    s.hover = Retained<CCNode>(hits.empty() ? nullptr : hits[s.cycle % hits.size()]);
    return s.hover;
  }

  ListenerResult onMouse(MouseInputData& data)
  {
    auto& s = state();
    s.pinned.reset();
    auto mouse = getMousePos();
    using enum MouseInputData::Action;
    using enum MouseInputData::Button;

    if (auto key = MouseInputData::buttonToKeyCode(data.button); key != KEY_None && isToggle(key, data.modifiers))
      return ListenerResult::Propagate;

    if (data.button == Left && data.action == Press)
    {
      s.leftDown = true;
      auto tree = s.overlay ? s.overlay->tree() : nullptr;
      if (tree)
        tree->setTyping(tree->searchAt(mouse));
      bool shift = hasModifier(data.modifiers, KeyboardModifier::Shift);
      auto handle = shift || (tree && tree->contains(mouse)) || (s.overlay && s.overlay->panelContains(mouse))
                        ? std::nullopt
                        : editor::handleAt(mouse);
      if (handle)
        editor::startResize(*handle, mouse);
      else if (tree && tree->contains(mouse))
      {
        if (auto node = tree->rowAt(mouse, runningScene()))
          editor::select(node);
      }
      else if (auto button = s.overlay ? s.overlay->buttonAt(mouse) : nullptr)
        button->action();
      else if (s.overlay && s.overlay->titleContains(mouse))
      {
        s.panelDragging = true;
        s.overlay->panelMoved();
        s.panelGrab = mouse - s.overlay->panelTopLeft();
      }
      else if (!s.overlay || !s.overlay->panelContains(mouse))
      {
        bool alt = hasModifier(data.modifiers, KeyboardModifier::Alt);
        // Shift+click: into the selection or out of it, no drag
        if (hasModifier(data.modifiers, KeyboardModifier::Shift))
        {
          auto hits = inspect::hitsAt(runningScene(), mouse, alt);
          if (ccpDistance(mouse, s.cyclePoint) > 1)
            s.cycle = 0;
          if (!hits.empty())
            editor::toggleSelect(hits[s.cycle % hits.size()]);
        }
        else
          editor::pressAt(mouse, alt);
      }
    }
    else if (data.button == Left && data.action == Release)
    {
      // A button held since before the edit mode: the game saw its press, it gets its release
      if (!s.leftDown)
        return ListenerResult::Propagate;
      s.leftDown = false;
      if (s.panelDragging && s.overlay)
        s.overlay->savePrefs();
      s.panelDragging = false;
      editor::release();
    }
    else if (data.button == Right && data.action == Press)
    {
      if (s.overlay && s.overlay->titleContains(mouse))
        s.overlay->panelUnplaced();
      else
        editor::select(nullptr);
    }
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

    // Values being typed take the keys
    if (s.typing)
    {
      editor::typingKey(data.key, shift);
      return ListenerResult::Stop;
    }

    // The tree's search takes the keys while it's focused
    auto tree = s.overlay ? s.overlay->tree() : nullptr;
    if (tree && tree->isVisible() && tree->typing())
    {
      if (data.key == KEY_Escape)
        tree->setTyping(false);
      else if (data.key == KEY_Enter || data.key == KEY_NumEnter)
      {
        if (auto node = tree->firstMatch(runningScene()))
          editor::select(node);
        tree->setTyping(false);
      }
      else if (data.key == KEY_Backspace)
        tree->erase();
      else if (auto c = tree::typed(data.key, shift))
        tree->type(*c);
      return ListenerResult::Stop;
    }

    // Alt with a letter lines the selected nodes up, as in Figma; with Shift, H and V distribute them
    if (hasModifier(data.modifiers, KeyboardModifier::Alt))
    {
      using align::Edge;
      switch (data.key)
      {
      case KEY_A:
        editor::alignTo(Edge::Left);
        break;
      case KEY_D:
        editor::alignTo(Edge::Right);
        break;
      case KEY_H:
        shift ? editor::distribute(true) : editor::alignTo(Edge::CenterX);
        break;
      case KEY_W:
        editor::alignTo(Edge::Top);
        break;
      case KEY_S:
        editor::alignTo(Edge::Bottom);
        break;
      case KEY_V:
        shift ? editor::distribute(false) : editor::alignTo(Edge::CenterY);
        break;
      default:
        break;
      }
      // Other keys with Alt are what they are without it: Alt+arrows nudge while the distances show
      switch (data.key)
      {
      case KEY_A:
      case KEY_D:
      case KEY_H:
      case KEY_W:
      case KEY_S:
      case KEY_V:
        return ListenerResult::Stop;
      default:
        break;
      }
    }

    switch (data.key)
    {
    case KEY_T:
      if (tree)
      {
        tree->setVisible(!tree->isVisible());
        s.overlay->savePrefs();
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
    // Ctrl with an arrow moves the anchor point instead, the node staying where it is
    case KEY_Left:
      control ? editor::stepAnchor(-1, 0) : editor::nudge(-step, 0);
      break;
    case KEY_Right:
      control ? editor::stepAnchor(1, 0) : editor::nudge(step, 0);
      break;
    case KEY_Up:
      control ? editor::stepAnchor(0, 1) : editor::nudge(0, step);
      break;
    case KEY_Down:
      control ? editor::stepAnchor(0, -1) : editor::nudge(0, -step);
      break;
    case KEY_LeftBracket:
      editor::stepZOrder(-1);
      break;
    case KEY_RightBracket:
      editor::stepZOrder(1);
      break;
    case KEY_Z:
      if (control)
        shift ? editor::redo() : editor::undo();
      break;
    case KEY_Y:
      if (control)
        editor::redo();
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
      if (s.overlay)
        s.overlay->savePrefs();
      break;
    case KEY_C:
      // Ctrl+C copies; C alone shows the code in the panel
      if (control)
        shift ? editor::copyAll() : editor::copySelected();
      else
      {
        s.codeView = !s.codeView;
        if (s.overlay)
          s.overlay->savePrefs();
      }
      break;
    case KEY_Enter:
    case KEY_NumEnter:
      editor::startTyping();
      break;
    case KEY_R:
      if (!control)
        editor::reset();
      break;
    case KEY_L:
      editor::copyLayout();
      break;
    case KEY_Delete:
      if (control)
        editor::forgetKept();
      break;
    case KEY_V:
      // Ctrl+V after Ctrl+C out of habit doesn't hide anything
      if (!control)
        editor::toggleVisible();
      break;
    case KEY_F1:
      if (s.overlay)
        s.overlay->toggleHelp();
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
    if (s.putBackScene == s.scene && now() >= s.putBackAt && !typeinfo_cast<CCTransitionScene*>(runningScene()))
    {
      s.putBackScene = 0;
      putBack();
    }
    saveSession(false);
    // A level hides the cursor and, with GD's lock cursor option, locks it, which also stops the mouse's position
    // from changing; the level does it again when it's resumed, so it's undone every frame
    // Both read first: showing the cursor also lets it go
    auto view = CCEGLView::get();
    bool hidden = view->getShouldHideCursor();
    bool locked = view->getCursorLocked();
    if (hidden)
    {
      view->showCursor(true);
      s.rehideCursor = true;
    }
    if (locked)
    {
      view->toggleLockCursor(false);
      s.relockCursor = true;
    }
    // Removed, or the scene changed
    std::erase_if(s.also, [](auto const& node) { return !inspect::isIn(node.get(), runningScene()); });
    if (s.selected && !inspect::isIn(s.selected.get(), runningScene()))
    {
      s.selected.reset();
      if (!s.also.empty())
      {
        s.selected = s.also.front();
        s.also.erase(s.also.begin());
      }
      s.typing.reset();
      s.pressed = s.dragging = false;
    }

    auto mouse = mousePosition();
    // A release lost to the window losing focus (Geode drops the input queued meanwhile): the drag ends with it
    if ((s.pressed || s.panelDragging || s.resizing) && !s.pinned && GetActiveWindow() == nullptr)
    {
      s.leftDown = false;
      s.panelDragging = false;
      editor::release();
    }
    if (s.panelDragging)
      movePanel(mouse - s.panelGrab);
    else if (s.resizing && !s.pinned)
      editor::resizeTo(mouse);
    else if (s.pressed && !s.pinned)
      editor::dragTo(mouse);
    auto hover = hovered(mouse);
    if (m_tree->isVisible())
      m_tree->refresh(runningScene(), s.selected.get(), hover.get(), editor::editedNodes());
    else
      m_tree->forget();
    redraw(hover.get(), s.selected.get());
    // Not over the node being looked at, unless it's being dragged (the panel jumping under the mouse would be worse)
    if (!s.dragging)
      dodge(s.selected.get());
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
    s.putBackScene = s.scene;
    s.putBackAt = now() + 0.1;
    if (CCIMEDispatcher::sharedDispatcher()->hasDelegate())
    {
      unfocusTextFields(runningScene());
      log::info("A text field had the keyboard: {}", CCIMEDispatcher::sharedDispatcher()->hasDelegate() ? "still" : "let go");
    }
    s.codeView = Mod::get()->getSavedValue<bool>("code", false);
    s.aspectPreview = Mod::get()->getSavedValue<bool>("aspect", false);
    s.mouse = MouseInputEvent().listen(onMouse, PRIORITY);
    s.move = MouseMoveEvent().listen(
        [](int32_t, int32_t)
        {
          auto& s = state();
          s.pinned.reset();
          // The game gets no moves of a drag it never saw start
          return s.dragging || s.panelDragging || s.resizing ? ListenerResult::Stop : ListenerResult::Propagate;
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
          if (CCKeyboardDispatcher::get()->getControlKeyPressed())
            editor::turn(static_cast<float>(y), fine);
          else
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
    s.putBackScene = 0;
    s.sessionDirty = true;
    saveSession(true);
    // The cursor as the level had it, if the level is still being played (paused, the game shows it itself)
    auto running = runningScene();
    auto play = PlayLayer::get();
    bool playing = play && running && play->getParent() == running && !play->m_levelEndAnimationStarted &&
                   !running->getChildByType<PauseLayer>(0) && !running->getChildByType<EndLevelLayer>(0);
    if (s.rehideCursor && playing)
      CCEGLView::get()->showCursor(false);
    if (s.relockCursor && playing)
      CCEGLView::get()->toggleLockCursor(true);
    s.rehideCursor = s.relockCursor = false;
    s.pinned.reset();
    s.selected.reset();
    if (s.overlay)
    {
      s.overlay->removeFromParent();
      s.overlay = nullptr;
    }
    s.hover.reset();
    // A scene change while it's off would leave its nodes held until it's on again. Their code is kept
    auto scene = runningScene();
    fileAway([scene](CCNode* node) { return hasSteps(node) && !inspect::isIn(node, scene); });
    s.typing.reset();
    s.history.forget([scene](Retained<CCNode> const& node) { return !inspect::isIn(node.get(), scene); });
    std::erase_if(s.originals, [scene](auto const& entry) { return !inspect::isIn(entry.first.get(), scene); });
    pruneTracked();
    logger_link::clear();
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
  if (s.resizing)
    release();
  // A node of a selection of several: they're all dragged, this one first (snapping goes by it)
  auto part = std::find(s.also.begin(), s.also.end(), node);
  if (part != s.also.end())
  {
    std::swap(*part, s.selected);
  }
  else if (s.selected != node || !s.also.empty())
  {
    if (s.selected != node)
      select(node);
  }
  s.grabs.clear();
  if (node && node->getParent())
  {
    s.pressed = true;
    s.dragging = false;
    s.pressPoint = world;
    s.dragStart = inspect::stateOf(node);
    s.parentToWorld = inspect::toGeom(node->getParent()->nodeToWorldTransform());
    s.guides = guidesFor(node);
    for (auto other : selection())
      if (other->getParent())
        s.grabs.push_back({Retained<CCNode>(other), inspect::stateOf(other),
                           inspect::toGeom(other->getParent()->nodeToWorldTransform())});
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
  // Every selected node moved as far on the screen, each through its own parent's transform
  auto moveAll = [&](geom::Vec2 onScreen)
  {
    for (auto const& grab : s.grabs)
    {
      auto local = geom::localDelta(grab.parentToWorld, onScreen);
      grab.node->setPosition(grab.start.position + ccp(local.x, local.y));
    }
  };
  geom::Vec2 onScreen{delta.x, delta.y};
  moveAll(onScreen);

  // Ctrl held: no snapping and no grid, as in Figma. The node pressed snaps, the others follow it
  s.snapLines.clear();
  s.gridShown = 0;
  bool free = !s.snapOverride && !s.gridOverride && CCKeyboardDispatcher::get()->getControlKeyPressed();
  if (free)
    return;
  geom::Vec2 correction;
  if (s.snapOverride.value_or(Mod::get()->getSettingValue<bool>("snap")))
  {
    auto bounds = inspect::worldQuad(s.selected.get()).bounds();
    auto result = snap::snap(bounds, s.guides, SNAP_DISTANCE);
    if (result.correction.x != 0 || result.correction.y != 0)
    {
      correction = result.correction;
      moveAll(onScreen + correction);
      s.snapLines = std::move(result.lines);
    }
  }

  // The grid: on each axis no guide took, the pressed node's position as its code writes it (from the closest edge
  // or center of its frame) rounded to the step. Not for a node an axis layout places
  int step = s.gridOverride.value_or(static_cast<int>(Mod::get()->getSettingValue<int64_t>("grid")));
  auto node = s.selected.get();
  if (step <= 0 || !node->getParent())
    return;
  auto desc = inspect::describe(node);
  if (desc.layout.kind == codegen::LayoutKind::Axis || desc.layout.kind == codegen::LayoutKind::SimpleAxis)
    return;
  s.gridShown = step;
  auto position = codegen::positionExpressions(desc);
  geom::Vec2 local;
  if (correction.x == 0)
    local.x = expr::evaluate(grid::round(position.x, static_cast<float>(step)), desc.frame.extent.width) - desc.position.x;
  if (correction.y == 0)
    local.y = expr::evaluate(grid::round(position.y, static_cast<float>(step)), desc.frame.extent.height) - desc.position.y;
  if (local.x == 0 && local.y == 0)
    return;
  moveAll(onScreen + correction + s.parentToWorld.applyLinear(local));
}

void editor::setGrid(std::optional<int> step)
{
  state().gridOverride = step;
}

std::optional<CCPoint> editor::handleAt(CCPoint world)
{
  auto& s = state();
  if (!s.selected)
    return std::nullopt;
  // The closest handle within a few points
  std::optional<CCPoint> best;
  float closest = HANDLE_SIZE * 2.5f;
  for (auto const& [handle, at] : handles(s.selected.get()))
  {
    float distance = ccpDistance(world, at);
    if (distance <= closest)
    {
      closest = distance;
      best = handle;
    }
  }
  return best;
}

void editor::startResize(CCPoint handle, CCPoint world)
{
  auto& s = state();
  if (!s.selected)
    return;
  release();
  auto node = s.selected.get();
  s.resizing = State::Resizing{{handle.x, handle.y}, world, inspect::stateOf(node), inspect::transformOf(node),
                               inspect::toGeom(node->nodeToWorldTransform()).inverse()};
}

void editor::resizeTo(CCPoint world)
{
  auto& s = state();
  if (!s.resizing || !s.selected)
    return;
  auto node = s.selected.get();
  // The mouse's move in the node's own space, before its scale and turn
  auto delta = s.resizing->worldToNode.applyLinear({world.x - s.resizing->press.x, world.y - s.resizing->press.y});
  auto result = geom::resize(s.resizing->transform, s.resizing->handle, delta);
  // Not past what a typed value may be either, whatever a tiny scale makes of the mouse's move
  result.content.width = std::min(result.content.width, 1e6f);
  result.content.height = std::min(result.content.height, 1e6f);
  auto after = inspect::stateOf(node);
  after.position = CCPoint(result.position.x, result.position.y);
  after.size = CCSize(result.content.width, result.content.height);
  inspect::apply(node, after, true, false);
}

void editor::setSnapping(std::optional<bool> on)
{
  state().snapOverride = on;
}

CCPoint editor::snappedCenter(CCNode* node, CCPoint center)
{
  auto bounds = inspect::worldQuad(node).bounds();
  bounds.x += center.x - bounds.midX();
  bounds.y += center.y - bounds.midY();
  auto result = snap::snap(bounds, guidesFor(node), SNAP_DISTANCE);
  return center + ccp(result.correction.x, result.correction.y);
}

void editor::release()
{
  auto& s = state();
  s.snapLines.clear();
  s.gridShown = 0;
  if (s.resizing)
  {
    if (s.selected)
      record(s.selected.get(), s.resizing->start, Resize, 0);
    s.resizing.reset();
  }
  if (s.dragging && s.selected)
  {
    int group = s.grabs.size() > 1 ? ++s.groups : 0;
    for (auto const& grab : s.grabs)
      record(grab.node.get(), grab.start, Drag, 0, group);
    s.lastGroup = {};
  }
  s.grabs.clear();
  s.pressed = false;
  s.dragging = false;
}

void editor::wheel(float notches, bool fine)
{
  auto& s = state();
  if (!s.selected || notches == 0 || s.dragging || s.resizing)
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
  (void)before;
  editAll(Wheel, WHEEL_MERGE,
          [&](CCNode* each)
          {
            auto after = inspect::stateOf(each);
            after.scaleX = scaled(after.scaleX);
            after.scaleY = scaled(after.scaleY);
            // apply sets a button's base scale too, which its press animation goes back to
            inspect::apply(each, after, false, false);
          });
}

void editor::turn(float notches, bool fine)
{
  auto& s = state();
  if (!s.selected || notches == 0 || s.dragging || s.resizing)
    return;
  // The wheel up turns counterclockwise, as a knob would; Cocos' rotation goes clockwise
  editAll(Turn, WHEEL_MERGE,
          [&](CCNode* each)
          {
            auto after = inspect::stateOf(each);
            after.rotation = std::round((after.rotation - notches * (fine ? 1.f : 15.f)) * 100) / 100;
            inspect::apply(each, after, false, false);
          });
}

void editor::nudge(float dx, float dy)
{
  auto& s = state();
  if (!s.selected || s.dragging || s.resizing)
    return;
  editAll(Nudge, NUDGE_MERGE, [&](CCNode* each) { each->setPosition(each->getPosition() + ccp(dx, dy)); });
}

namespace
{
  // Each selected node moved by its own amount on the screen, through its parent's transform: one step
  // Each of `nodes` moved by its own amount on the screen, through its parent's transform: one step with the rest of
  // the selection, which doesn't move
  void moveOnScreen(Kind kind, std::vector<CCNode*> const& nodes, std::vector<geom::Vec2> const& moves)
  {
    editAll(kind, 0,
            [&](CCNode* node)
            {
              auto at = std::find(nodes.begin(), nodes.end(), node);
              if (at == nodes.end() || !node->getParent())
                return;
              auto move = moves[at - nodes.begin()];
              if (move.x == 0 && move.y == 0)
                return;
              auto local = geom::localDelta(inspect::toGeom(node->getParent()->nodeToWorldTransform()), move);
              node->setPosition(node->getPosition() + ccp(local.x, local.y));
            });
  }

  // The selected nodes an alignment moves: in the scene (a key can come before the frame forgets a removed one), and
  // not inside another selected one, which carries them along
  std::vector<CCNode*> alignable()
  {
    auto nodes = editor::selection();
    std::vector<CCNode*> out;
    for (auto node : nodes)
    {
      if (!node->getParent() || !inspect::isIn(node, runningScene()))
        continue;
      bool carried = false;
      for (auto up = node->getParent(); up && !carried; up = up->getParent())
        carried = std::find(nodes.begin(), nodes.end(), up) != nodes.end();
      if (!carried)
        out.push_back(node);
    }
    return out;
  }

  std::vector<geom::Rect> boxesOf(std::vector<CCNode*> const& nodes)
  {
    std::vector<geom::Rect> boxes;
    for (auto node : nodes)
      boxes.push_back(inspect::worldQuad(node).bounds());
    return boxes;
  }
} // namespace

void editor::alignTo(align::Edge edge)
{
  auto& s = state();
  if (!s.selected || s.pressed || s.resizing)
    return;
  auto nodes = alignable();
  if (nodes.empty())
    return;
  auto boxes = boxesOf(nodes);
  // Several: to the box of them all. One: to its parent's box, or the screen's if the parent has no size
  geom::Rect target = align::bounds(boxes);
  if (nodes.size() == 1)
  {
    auto parent = nodes[0]->getParent();
    auto win = CCDirector::get()->getWinSize();
    target = {0, 0, win.width, win.height};
    if (parent && parent != runningScene() && parent->getContentWidth() > 0 && parent->getContentHeight() > 0)
      target = inspect::worldQuad(parent).bounds();
  }
  moveOnScreen(Align, nodes, align::to(boxes, edge, target));
}

void editor::distribute(bool horizontal)
{
  auto& s = state();
  if (!s.selected || s.pressed || s.resizing)
    return;
  auto nodes = alignable();
  if (nodes.size() < 3)
    return;
  moveOnScreen(Align, nodes, align::distribute(boxesOf(nodes), horizontal));
}

void editor::toggleVisible()
{
  auto& s = state();
  if (!s.selected || s.pressed || s.resizing)
    return;
  // Hidden or shown as the first one is: all the same way
  bool show = !s.selected->isVisible();
  editAll(Visibility, 0, [show](CCNode* node) { node->setVisible(show); });
}

void editor::reset()
{
  auto& s = state();
  if (!s.selected || s.pressed || s.resizing)
    return;
  // Only nodes edited before have a state to go back to
  editAll(Reset, 0,
          [](CCNode* node)
          {
            if (auto original = originalOf(node))
            {
              auto const& s = state();
              auto in = [node](std::vector<CCNode*> const& list) { return std::find(list.begin(), list.end(), node) != list.end(); };
              inspect::apply(node, *original, isSized(node), in(s.toggled), turnedHere(node), fadedHere(node));
            }
          });
}

void editor::stepAnchor(int dx, int dy)
{
  auto& s = state();
  if (!s.selected || s.dragging || s.resizing)
    return;
  auto node = s.selected.get();
  auto before = inspect::stateOf(node);
  // To the next of 0, 0.5 and 1 on each axis, as far as 0 and 1
  auto next = [](float value, int direction)
  {
    if (direction == 0)
      return value;
    float stepped = direction > 0 ? std::floor(value * 2 + 1e-4f) / 2 + 0.5f : std::ceil(value * 2 - 1e-4f) / 2 - 0.5f;
    return std::clamp(stepped, 0.f, 1.f);
  };
  CCPoint anchor{next(before.anchor.x, dx), next(before.anchor.y, dy)};
  if (anchor == before.anchor)
    return;
  auto transform = inspect::transformOf(node);
  auto position = geom::positionForAnchor(transform, {anchor.x, anchor.y});
  node->setAnchorPoint(anchor);
  node->setPosition({position.x, position.y});
  record(node, before, Reanchor, 0);
}

void editor::stepZOrder(int delta)
{
  auto& s = state();
  if (!s.selected || s.dragging || s.resizing)
    return;
  auto node = s.selected.get();
  auto before = inspect::stateOf(node);
  node->setZOrder(before.zOrder + delta);
  record(node, before, Order, NUDGE_MERGE);
}

void editor::startTyping()
{
  auto& s = state();
  if (!s.selected || s.pressed || s.dragging || s.resizing)
    return;
  auto info = inspect::stateOf(s.selected.get());
  s.typing = fields::Fields({number(info.position.x), number(info.position.y), number(info.scaleX),
                             number(info.size.width), number(info.size.height), number(info.rotation),
                             std::to_string(info.opacity)});
  // What the fields start as, and the exact values behind them: a field left as it was keeps its exact value,
  // not the rounded one it shows
  s.typingStart = s.typing->values();
  s.typingExact = {info.position.x,  info.position.y, info.scaleX,
                   info.size.width, info.size.height, info.rotation,
                   static_cast<float>(info.opacity)};
  s.typingError.clear();
}

bool editor::typing()
{
  return state().typing.has_value();
}

void editor::typingKey(int key, bool shift)
{
  auto& s = state();
  if (!s.typing)
    return;
  if (!s.selected || key == KEY_Escape)
  {
    s.typing.reset();
    return;
  }
  if (key == KEY_Tab)
    shift ? s.typing->previous() : s.typing->next();
  else if (key == KEY_Backspace)
    s.typing->erase();
  else if (key == KEY_Enter || key == KEY_NumEnter)
  {
    auto node = s.selected.get();
    auto win = CCDirector::get()->getWinSize();
    auto parent = node->getParent() ? node->getParent()->getContentSize() : win;
    auto values = s.typing->evaluate([&](std::string_view name) -> std::optional<float>
                                     {
                                       if (name == "w")
                                         return win.width;
                                       if (name == "h")
                                         return win.height;
                                       if (name == "pw")
                                         return parent.width;
                                       if (name == "ph")
                                         return parent.height;
                                       return std::nullopt;
                                     });
    for (size_t i = 0; i < values.size(); i++)
    {
      if (i < s.typingStart.size() && s.typing->values()[i] == s.typingStart[i])
        values[i] = s.typingExact[i];
      // A number too big to be one, or a scale the node would vanish at and the wheel couldn't bring back
      bool bad = !values[i] || !std::isfinite(*values[i]) || std::fabs(*values[i]) > 1e6f ||
                 (i == 2 && std::fabs(*values[2]) < 0.01f) || ((i == 3 || i == 4) && *values[i] < 0) ||
                 (i == 6 && (*values[6] < 0 || *values[6] > 255));
      if (bad)
      {
        s.typingError = s.typing->values()[i];
        return;
      }
    }
    auto before = inspect::stateOf(node);
    auto after = before;
    after.position = CCPoint(*values[0], *values[1]);
    // A node scaled unevenly keeps its proportions
    float ratio = before.scaleX != 0 ? before.scaleY / before.scaleX : 1;
    after.scaleX = *values[2];
    after.scaleY = *values[2] * ratio;
    after.size = CCSize(*values[3], *values[4]);
    after.rotation = *values[5];
    after.opacity = static_cast<int>(std::round(*values[6]));
    inspect::apply(node, after, true, false);
    record(node, before, Typed, 0);
    s.typing.reset();
  }
  else if (auto c = fields::typed(key, shift))
    s.typing->type(*c);
  s.typingError.clear();
}

bool editor::undo()
{
  auto& s = state();
  release();
  s.lastGroup = {};
  while (true)
  {
    auto changes = s.history.undoGroup();
    if (changes.empty())
      return false;
    std::vector<CCNode*> undone;
    for (auto const& change : changes)
    {
      auto node = change.target.get();
      // Removed since: nothing to put back, and no code to keep for it
      if (!inspect::isIn(node, runningScene()))
      {
        std::erase_if(s.live, [&](auto const& entry) { return entry.first == node; });
        continue;
      }
      inspect::apply(node, change.before, change.before.size != change.after.size,
                     change.before.visible != change.after.visible, change.before.rotation != change.after.rotation,
                     change.before.opacity != change.after.opacity);
      snapshot(node);
      undone.push_back(node);
      log::info("Undid a step of {}: position ({}, {}), scale {} x {}", inspect::name(node),
                number(change.before.position.x), number(change.before.position.y), number(change.before.scaleX),
                number(change.before.scaleY));
    }
    if (undone.empty())
      continue;
    selectAll(undone);
    return true;
  }
}

bool editor::redo()
{
  auto& s = state();
  release();
  s.lastGroup = {};
  while (true)
  {
    auto changes = s.history.redoGroup();
    if (changes.empty())
      return false;
    std::vector<CCNode*> redone;
    for (auto const& change : changes)
    {
      auto node = change.target.get();
      if (!inspect::isIn(node, runningScene()))
        continue;
      inspect::apply(node, change.after, change.before.size != change.after.size,
                     change.before.visible != change.after.visible, change.before.rotation != change.after.rotation,
                     change.before.opacity != change.after.opacity);
      snapshot(node);
      redone.push_back(node);
      log::info("Redid a step of {}", inspect::name(node));
    }
    if (redone.empty())
      continue;
    selectAll(redone);
    return true;
  }
}

void editor::select(CCNode* node)
{
  auto& s = state();
  s.also.clear();
  if (s.selected == node)
    return;
  release();
  s.typing.reset();
  s.selected = Retained<CCNode>(node);
  if (node)
    log::info("Selected {}", inspect::name(node));
}

void editor::toggleSelect(CCNode* node)
{
  auto& s = state();
  if (!node)
    return;
  release();
  s.typing.reset();
  s.lastGroup = {};
  auto part = std::find(s.also.begin(), s.also.end(), node);
  if (s.selected == node)
  {
    // The first of the others is shown instead
    if (s.also.empty())
      s.selected.reset();
    else
    {
      s.selected = s.also.front();
      s.also.erase(s.also.begin());
    }
  }
  else if (part != s.also.end())
    s.also.erase(part);
  else if (!s.selected)
    s.selected = Retained<CCNode>(node);
  else
    s.also.emplace_back(node);
  log::info("Selection: {} nodes", selection().size());
}

std::vector<CCNode*> editor::selection()
{
  auto& s = state();
  std::vector<CCNode*> nodes;
  if (s.selected)
    nodes.push_back(s.selected.get());
  for (auto const& node : s.also)
    nodes.push_back(node.get());
  return nodes;
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
  return styled(codegen::generate(describeEdited(node)));
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

namespace
{
  // Every edit for "Copy all", by layer: the scenes left, and this one's merged into its layer's group (a node
  // edited again replacing its older code). This scene's nodes described as they are now; nodes removed from it
  // with their edits as they were last
  std::vector<std::pair<std::string, std::vector<codegen::NodeDesc>>> allEdits()
  {
    auto& s = state();
    auto groups = s.earlier;
    for (auto const& [node, kept] : s.live)
    {
      if (!hasSteps(node))
        continue;
      auto desc = inspect::isIn(node, runningScene()) ? describeEdited(node) : kept;
      // By the layer whose hook the code goes in
      auto layer = desc.layer.empty() ? layerName(runningScene()) : desc.layer;
      auto group = std::find_if(groups.begin(), groups.end(), [&](auto const& g) { return g.first == layer; });
      if (group == groups.end())
      {
        groups.emplace_back(layer, std::vector<codegen::NodeDesc>{});
        group = groups.end() - 1;
      }
      addTo(group->second, desc);
    }
    std::erase_if(groups, [](auto const& g) { return g.second.empty(); });
    return groups;
  }
} // namespace

std::string editor::codeForAll()
{
  auto groups = allEdits();
  if (groups.empty())
    return "";
  // Each layer under its name, as the hook its code goes in: ready to paste for the layers mods hook most
  // Geode's header and namespace first, for a file of its own
  std::string code = hooks::preamble();
  for (auto const& [layer, descs] : groups)
    code += "\n// ! --- In " + layer + " --- !\n\n" + hooks::wrap(layer, codegen::generateAll(descs));
  return styled(code);
}

size_t editor::editedCount()
{
  size_t count = 0;
  for (auto const& [layer, descs] : allEdits())
    count += descs.size();
  return count;
}

namespace
{
  // The selected nodes as a layout: two or more of one parent, their bounding boxes in its space
  std::optional<codegen::LayoutDesc> selectedLayout()
  {
    auto nodes = editor::selection();
    if (nodes.size() < 2)
      return std::nullopt;
    auto parent = nodes[0]->getParent();
    if (!parent || std::any_of(nodes.begin(), nodes.end(), [&](CCNode* node) { return node->getParent() != parent; }))
      return std::nullopt;
    codegen::LayoutDesc layout;
    std::vector<geom::Rect> boxes;
    for (auto node : nodes)
    {
      auto box = node->boundingBox();
      boxes.push_back({box.origin.x, box.origin.y, box.size.width, box.size.height});
      layout.nodes.push_back(describeEdited(node));
    }
    // Only a menu lets a button be pressed
    layout.menu =
        std::any_of(nodes.begin(), nodes.end(), [](CCNode* node) { return typeinfo_cast<CCMenuItem*>(node) != nullptr; });
    layout.fit = rowfit::fit(boxes);
    layout.parent = inspect::describe(parent);
    layout.parentIsLayer = inspect::className(parent) == layout.nodes[0].layer &&
                           (hooks::find(layout.nodes[0].layer) || typeinfo_cast<CCScene*>(parent->getParent()));
    return layout;
  }
} // namespace

std::string editor::layoutCode()
{
  auto layout = selectedLayout();
  return layout ? styled(codegen::layoutCode(*layout)) : "";
}

void editor::copyLayout()
{
  inspect::forgetLookups();
  auto layout = selectedLayout();
  if (!layout)
  {
    Notification::create(ui::words().layoutNeeds, NotificationIcon::Warning, 2.f)->show();
    return;
  }
  auto code = styled(codegen::layoutCode(*layout));
  auto count = std::to_string(layout->nodes.size());
  clipboard::write(code);
  log::info("Layout for {} selected nodes:\n{}", count, code);
  logger_link::mark("copied a layout for " + count + " nodes");
  auto const& w = ui::words();
  Notification::create(text::format(w.copiedLayout, {layout->fit.row ? w.row : w.column, count}), NotificationIcon::Success,
                       1.5f)
      ->show();
}

namespace
{
  std::filesystem::path sessionPath()
  {
    return Mod::get()->getSaveDir() / (selftest::requested() ? "session-selftest.txt" : "session.txt");
  }

  // The kept edits written again if they changed, at most every couple of seconds unless `now`
  void saveSession(bool force)
  {
    auto& s = state();
    if (!s.sessionDirty || (!force && now() - s.sessionSaved < 2))
      return;
    s.sessionDirty = false;
    s.sessionSaved = now();
    auto written = file::writeString(sessionPath(), session::write(allEdits()));
    if (!written)
      log::warn("Couldn't write {}: {}", sessionPath(), written.unwrapErr());
  }

  // The kept edits of a layer made again put on its nodes, found as their code finds them: one step to undo, a node
  // already as they say (its code pasted since) has nothing to put back. Only with the setting on
  void putBack()
  {
    auto& s = state();
    if (!s.putBackOverride.value_or(Mod::get()->getSettingValue<bool>("put-back")))
      return;
    auto scene = runningScene();
    int group = ++s.groups;
    size_t count = 0;
    for (auto& [layer, descs] : s.earlier)
      std::erase_if(descs,
                    [&](codegen::NodeDesc const& desc)
                    {
                      auto node = inspect::resolve(scene, desc);
                      if (!node || hasSteps(node))
                        return false;
                      auto before = inspect::stateOf(node);
                      auto wanted = before;
                      wanted.position = CCPoint{desc.position.x, desc.position.y};
                      if (desc.scaleChanged)
                      {
                        wanted.scaleX = desc.scaleX;
                        wanted.scaleY = desc.scaleY;
                      }
                      if (desc.anchorChanged)
                        wanted.anchor = CCPoint{desc.anchor.x, desc.anchor.y};
                      if (desc.zOrderChanged)
                        wanted.zOrder = desc.zOrder;
                      if (desc.sizeChanged)
                      {
                        wanted.size = CCSize{desc.size.width, desc.size.height};
                        s.sized.push_back(node);
                      }
                      if (desc.visibleChanged)
                      {
                        wanted.visible = desc.visible;
                        s.toggled.push_back(node);
                      }
                      if (desc.rotationChanged)
                        wanted.rotation = desc.rotation;
                      if (desc.opacityChanged)
                        wanted.opacity = desc.opacity;
                      inspect::apply(node, wanted, desc.sizeChanged, desc.visibleChanged, desc.rotationChanged,
                                     desc.opacityChanged);
                      record(node, before, PutBack, 0, group);
                      // Now this scene's, as live as any edit; kept here too if nothing changed (already in place)
                      if (!hasSteps(node))
                        return false;
                      count++;
                      return true;
                    });
    std::erase_if(s.earlier, [](auto const& g) { return g.second.empty(); });
    if (count > 0)
    {
      log::info("Put back the kept edits of {} nodes", count);
      Notification::create(text::format(ui::words().putBack, {std::to_string(count)}), NotificationIcon::Info, 1.5f)
          ->show();
    }
  }
} // namespace

void editor::loadSession()
{
  auto text = file::readString(sessionPath());
  if (!text)
    return;
  auto groups = session::read(*text);
  if (!groups)
  {
    log::warn("{} isn't a session file", sessionPath());
    return;
  }
  auto& s = state();
  for (auto const& [layer, descs] : *groups)
    for (auto const& desc : descs)
      addTo(groupOf(layer), desc);
  size_t count = 0;
  for (auto const& [layer, descs] : *groups)
    count += descs.size();
  if (count > 0)
    log::info("Kept edits of {} nodes from the last time", count);
  s.sessionDirty = false;
}

void editor::saveSessionNow()
{
  state().sessionDirty = true;
  saveSession(true);
}

std::string editor::sessionFile()
{
  return sessionPath().string();
}

void editor::setPutBack(std::optional<bool> on)
{
  state().putBackOverride = on;
}

void editor::forgetKept()
{
  auto& s = state();
  size_t count = 0;
  for (auto const& [layer, descs] : s.earlier)
    count += descs.size();
  s.earlier.clear();
  s.sessionDirty = true;
  saveSession(true);
  log::info("Forgot the kept edits of {} nodes", count);
  Notification::create(text::format(ui::words().forgotKept, {std::to_string(count)}), NotificationIcon::Info, 1.5f)
      ->show();
}

void editor::copySelected()
{
  inspect::forgetLookups();
  auto node = selected();
  if (!node)
  {
    Notification::create(ui::words().selectFirst, NotificationIcon::Warning, 1.5f)->show();
    return;
  }
  // Several selected: the code for all of them in one block
  auto nodes = selection();
  if (nodes.size() > 1)
  {
    std::vector<codegen::NodeDesc> descs;
    for (auto each : nodes)
      descs.push_back(describeEdited(each));
    auto code = styled(codegen::generateAll(descs));
    clipboard::write(code);
    log::info("Code for {} selected nodes:\n{}", nodes.size(), code);
    Notification::create(text::format(ui::words().copiedNodes, {std::to_string(nodes.size())}), NotificationIcon::Success,
                         1.5f)
        ->show();
    return;
  }
  auto code = codeFor(node);
  clipboard::write(code);
  log::info("Code for {}:\n{}", inspect::name(node), code);
  logger_link::mark("copied the code for " + inspect::name(node));
  Notification::create(text::format(ui::words().copiedFor, {inspect::name(node)}), NotificationIcon::Success, 1.5f)
      ->show();
}

void editor::copyAll()
{
  inspect::forgetLookups();
  auto count = editedCount();
  if (count == 0)
  {
    Notification::create(ui::words().nothingEdited, NotificationIcon::Warning, 1.5f)->show();
    return;
  }
  auto code = codeForAll();
  clipboard::write(code);
  // Also a file, for a long session's worth of edits
  auto path = Mod::get()->getSaveDir() / "export.cpp";
  auto written = file::writeString(path, code);
  if (!written)
    log::warn("Couldn't write {}: {}", path, written.unwrapErr());
  log::info("Code for {} edited nodes{}:\n{}", count,
            written ? fmt::format(" (also in {})", path) : std::string(), code);
  logger_link::mark(fmt::format("copied the code for {} nodes", count));
  Notification::create(text::format(ui::words().copiedNodes, {std::to_string(count)}), NotificationIcon::Success, 1.5f)
      ->show();
}

// ! --- Self test --- !

void editor::setMeasuring(std::optional<bool> on)
{
  state().measureOverride = on;
}

std::vector<float> editor::measurements()
{
  std::vector<float> lengths;
  for (auto const& line : state().measured)
    lengths.push_back(line.length);
  return lengths;
}

void editor::setTreeVisible(bool on)
{
  if (auto overlay = state().overlay)
    overlay->tree()->setVisible(on);
}

void editor::setCodeView(bool on)
{
  state().codeView = on;
}

void editor::setAspectPreview(bool on)
{
  state().aspectPreview = on;
}

void editor::pinMouse(std::optional<CCPoint> world)
{
  state().pinned = world;
}

bool editor::overPanel(CCPoint world)
{
  auto& s = state();
  return s.overlay && s.overlay->panelContains(world);
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
