#include "SelfTest.hpp"

#include "Editor.hpp"
#include "Inspect.hpp"
#include "Retained.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/MenuLayer.hpp>

#include <cmath>
#include <cstdlib>

using namespace geode::prelude;

namespace
{
  // Frames in the main menu before the test, for it to settle
  constexpr int SETTLE_FRAMES = 90;
  constexpr float DRAG_X = 30.f;
  constexpr float DRAG_Y = -20.f;

  int g_checks = 0;
  int g_failed = 0;
  int g_frames = 0;

  void check(bool ok, std::string const& what)
  {
    g_checks++;
    if (ok)
      return;
    g_failed++;
    log::error("Self test FAIL: {}", what);
  }

  bool close(float a, float b, float epsilon = 0.01f)
  {
    return std::fabs(a - b) <= epsilon;
  }

  CCPoint worldPositionOf(CCNode* node)
  {
    return node->getParent()->convertToWorldSpace(node->getPosition());
  }

  bool visibleInTree(CCNode* node)
  {
    for (; node; node = node->getParent())
      if (!node->isVisible())
        return false;
    return true;
  }

  // The scene in the log, for when the test can't find what it looks for
  void dumpTree(CCNode* node, int depth)
  {
    if (depth > 6)
      return;
    auto size = node->getContentSize();
    log::info("Self test tree: {}{} visible {} size {}x{}", std::string(depth * 2, ' '), inspect::name(node),
              node->isVisible(), size.width, size.height);
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      dumpTree(child, depth + 1);
  }

  // Whether the mouse can get to it: under its own middle, with Alt+click if it's inside a button
  bool reachable(CCNode* node)
  {
    auto center = inspect::worldQuad(node).center();
    auto hits = inspect::hitsAt(CCDirector::get()->getRunningScene(), {center.x, center.y}, true);
    return std::find(hits.begin(), hits.end(), node) != hits.end();
  }

  // A label (bitmap or TTF) on the screen: one with an ID outside layouts if there is one, then one with an ID, then any
  CCNode* findLabel(CCNode* root)
  {
    CCNode* inLayout = nullptr;
    CCNode* withoutId = nullptr;
    std::vector<CCNode*> queue{root};
    for (size_t i = 0; i < queue.size(); i++)
    {
      auto node = queue[i];
      if (typeinfo_cast<CCLabelProtocol*>(node))
      {
        if (!reachable(node))
          continue;
        if (node->getID().empty())
          withoutId = withoutId ? withoutId : node;
        else if (!node->getParent()->getLayout())
          return node;
        else
          inLayout = inLayout ? inLayout : node;
        // Its letters aren't labels
        continue;
      }
      for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
        if (child->isVisible())
          queue.push_back(child);
    }
    return inLayout ? inLayout : withoutId;
  }

  struct Case
  {
    Retained<CCNode> node;
    inspect::NodeState before;
    inspect::NodeState after;
  };

  // Press in the middle, drag in steps, let go, two notches of the wheel, an arrow and a Shift+arrow
  void edit(Case& item)
  {
    auto node = item.node.get();
    auto name = inspect::name(node);
    item.before = inspect::stateOf(node);
    auto center = inspect::worldQuad(node).center();
    CCPoint press{center.x, center.y};
    auto worldBefore = worldPositionOf(node);

    // What's under the middle of it, topmost first: other mods may cover or hide the game's nodes
    auto scene = CCDirector::get()->getRunningScene();
    auto hits = inspect::hitsAt(scene, press);
    std::string under;
    for (size_t i = 0; i < hits.size() && i < 5; i++)
      under += (i ? ", " : "") + inspect::name(hits[i]);
    log::info("Self test: under the middle of {}: {}", name, under.empty() ? "nothing" : under);

    editor::pinMouse(press);
    auto at = std::find(hits.begin(), hits.end(), node);
    // Inside a button it's the button that's picked; Alt+click goes inside
    bool deep = false;
    if (at == hits.end())
    {
      auto inside = inspect::hitsAt(scene, press, true);
      if (std::find(inside.begin(), inside.end(), node) != inside.end())
      {
        log::info("Self test: {} is part of {}, Alt+click", name, hits.empty() ? "nothing" : inspect::name(hits[0]));
        hits = inside;
        at = std::find(hits.begin(), hits.end(), node);
        deep = true;
      }
    }
    if (!visibleInTree(node))
    {
      // Hidden by another mod: nothing to pick, edited all the same
      log::info("Self test: {} is hidden (another mod?), grabbed without the mouse", name);
      editor::grab(node, press);
    }
    else
    {
      check(at != hits.end(), fmt::format("{} is under the mouse in its middle", name));
      // Covered by something: Tab until it's the one, as a user would
      for (auto i = at - hits.begin(); at != hits.end() && i > 0; i--)
        editor::nextUnderMouse();
      auto picked = editor::pressAt(press, deep);
      check(picked == node, fmt::format("the mouse in the middle of {} picks it, after {} Tab (picked {})", name,
                                        at - hits.begin(), picked ? inspect::name(picked) : "nothing"));
      if (picked != node)
      {
        editor::release();
        editor::grab(node, press);
      }
    }
    for (int step = 1; step <= 5; step++)
    {
      auto at = press + ccp(DRAG_X, DRAG_Y) * (step / 5.f);
      editor::pinMouse(at);
      editor::dragTo(at);
    }
    editor::release();
    auto moved = worldPositionOf(node) - worldBefore;
    check(close(moved.x, DRAG_X, 0.05f) && close(moved.y, DRAG_Y, 0.05f),
          fmt::format("the drag moved {} by ({}, {}) on the screen, not ({}, {})", name, moved.x, moved.y, DRAG_X,
                      DRAG_Y));

    auto dragged = inspect::stateOf(node);
    editor::wheel(2, false);
    auto scaled = inspect::stateOf(node);
    check(scaled.scaleX > dragged.scaleX && scaled.scaleY > dragged.scaleY,
          fmt::format("the wheel made {} bigger ({} to {})", name, dragged.scaleX, scaled.scaleX));
    check(scaled.position == dragged.position, fmt::format("the wheel didn't move {}", name));

    editor::nudge(1, 0);
    editor::nudge(10, 0);
    item.after = inspect::stateOf(node);
    check(close(item.after.position.x, dragged.position.x + 11) && close(item.after.position.y, dragged.position.y),
          fmt::format("the arrows moved {} by 11 in its parent", name));

    // Why it would act oddly: a hidden node has to say what hides it
    auto facts = inspect::factsOf(node);
    for (auto const& reason : explain::reasons(facts))
      log::info("Self test: {}: {}", name, reason);
    check(visibleInTree(node) == facts.hiddenBy.empty(),
          fmt::format("{} is said to be hidden exactly when it is ({})", name, facts.hiddenBy));

    auto code = editor::codeFor(node);
    log::info("Self test: the code for {}:\n{}", name, code);
    auto problem = inspect::checkCode(node, code);
    check(problem.empty(), fmt::format("the code for {} puts it where it is: {}", name, problem));
  }

  // A drag that ends with the node's center a point off the screen's middle ends right on it; without snapping
  // it doesn't. One step to undo either way
  void finish(bool measurable);

  void snapping(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto win = CCDirector::get()->getWinSize();
    for (bool on : {true, false})
    {
      editor::setSnapping(on);
      auto bounds = inspect::worldQuad(node).bounds();
      CCPoint from{bounds.midX(), bounds.midY()};
      // Halfway up the screen too, so it snaps on both axes
      CCPoint to{win.width / 2 + 1, win.height / 2 - 1};
      editor::pinMouse(from);
      editor::grab(node, from);
      for (int step = 1; step <= 4; step++)
        editor::dragTo(from + (to - from) * (step / 4.f));
      editor::release();
      auto after = inspect::worldQuad(node).bounds();
      if (on)
        check(close(after.midX(), win.width / 2) && close(after.midY(), win.height / 2),
              fmt::format("{} snapped to the middle of the screen: its center at ({}, {})", name, after.midX(),
                          after.midY()));
      else
        check(close(after.midX(), win.width / 2 + 1, 0.05f) && close(after.midY(), win.height / 2 - 1, 0.05f),
              fmt::format("{} didn't snap with snapping off: its center at ({}, {})", name, after.midX(),
                          after.midY()));
      check(editor::undo(), "the snapped drag is one step to undo");
      check(inspect::stateOf(node) == before, fmt::format("undo put {} back after the snapped drag", name));
    }
    editor::setSnapping(std::nullopt);
  }

  void run(MenuLayer* layer)
  {
    log::info("Self test: start");
    editor::setActive(true);
    // The plain drags are checked to the point: no snapping in them
    editor::setSnapping(false);
    check(editor::active(), "the edit mode is on");
    check(editor::swallowsClick(), "a click in the edit mode is kept from the game");

    std::vector<Case> cases;
    for (auto id : {"play-button", "geode.loader/geode-button"})
    {
      auto node = layer->getChildByIDRecursive(id);
      check(node != nullptr, fmt::format("{} is in the main menu", id));
      if (node)
        cases.push_back({Retained<CCNode>(node), {}, {}});
    }
    // Another mod's menu may be beside MenuLayer in the scene, not in it
    auto label = findLabel(CCDirector::get()->getRunningScene());
    check(label != nullptr, "a label is on the screen");
    if (!label)
      dumpTree(CCDirector::get()->getRunningScene(), 0);
    if (label)
      cases.push_back({Retained<CCNode>(label), {}, {}});

    for (auto& item : cases)
      edit(item);

    auto edited = editor::editedNodes();
    check(edited.size() == cases.size(), fmt::format("{} nodes edited, {} expected", edited.size(), cases.size()));
    auto all = editor::codeForAll();
    log::info("Self test: the code for every edited node:\n{}", all);
    for (auto& item : cases)
    {
      auto id = std::string(item.node->getID());
      check(all.find("// " + inspect::name(item.node.get())) != std::string::npos,
            fmt::format("the code for all has {}", inspect::name(item.node.get())));
    }

    int undone = 0;
    while (editor::undo())
      undone++;
    log::info("Self test: {} steps undone", undone);
    // A drag, the wheel and the two arrows (merged) for each
    check(undone == static_cast<int>(cases.size()) * 3, fmt::format("{} steps undone, {} expected", undone,
                                                                     cases.size() * 3));
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.before,
            fmt::format("undo put {} back", inspect::name(item.node.get())));

    int redone = 0;
    while (editor::redo())
      redone++;
    check(redone == undone, fmt::format("{} steps redone, {} undone", redone, undone));
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.after,
            fmt::format("redo put {} where it was edited to", inspect::name(item.node.get())));
    while (editor::undo())
    {
    }
    for (auto& item : cases)
      check(inspect::stateOf(item.node.get()) == item.before,
            fmt::format("undo after redo put {} back", inspect::name(item.node.get())));

    std::vector<CCNode*> shown;
    for (auto& item : cases)
      if (visibleInTree(item.node.get()))
        shown.push_back(item.node.get());
    if (!shown.empty())
      snapping(shown[0]);

    // For the screenshot: the first visible one selected, the mouse over the next visible one
    if (!shown.empty())
      editor::select(shown[0]);
    // The mouse over a visible sibling of the selected one if there is one (the panel covers the top left), or
    // over the next node
    CCNode* other = nullptr;
    if (!shown.empty())
      for (auto sibling : CCArrayExt<CCNode*>(shown[0]->getParent()->getChildren()))
        if (sibling != shown[0] && visibleInTree(sibling) && sibling->getContentWidth() > 0 && reachable(sibling))
        {
          other = sibling;
          break;
        }
    if (!other && shown.size() > 1)
      other = shown[1];
    if (other)
    {
      auto center = inspect::worldQuad(other).center();
      editor::pinMouse(CCPoint{center.x, center.y});
    }

    // UI_TWEAKER_SELFTEST=aspect: the other screens' shapes on, for a screenshot of them
    if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "aspect")
      editor::setAspectPreview(true);

    // The distances as with Alt held, from the selected node to the one under the mouse: the overlay measures
    // them when it draws, so the result is a few frames away
    editor::setMeasuring(true);
    g_frames = 0;
    finish(other != nullptr);
  }

  void finish(bool measurable)
  {
    if (++g_frames < 5)
    {
      Loader::get()->queueInMainThread([measurable] { finish(measurable); });
      return;
    }
    if (measurable)
    {
      auto lengths = editor::measurements();
      std::string text;
      for (auto length : lengths)
        text += fmt::format("{} ", length);
      log::info("Self test: distances from the selected node to the hovered one: {}", text);
      check(!lengths.empty() && std::all_of(lengths.begin(), lengths.end(), [](float l) { return l > 0; }),
            "Alt shows the distances from the selected node to the hovered one");
    }

    if (g_failed == 0)
      log::info("Self test PASS: {} checks", g_checks);
    else
      log::error("Self test FAIL: {} of {} checks failed", g_failed, g_checks);
  }

  void poll()
  {
    auto scene = CCDirector::get()->getRunningScene();
    auto layer = scene ? scene->getChildByType<MenuLayer>(0) : nullptr;
    if (!layer)
      g_frames = 0;
    else if (++g_frames >= SETTLE_FRAMES)
    {
      run(layer);
      return;
    }
    Loader::get()->queueInMainThread(poll);
  }
} // namespace

bool selftest::requested()
{
  auto value = std::getenv("UI_TWEAKER_SELFTEST");
  return value && *value && std::string(value) != "0";
}

void selftest::start()
{
  log::info("Self test: waiting for the main menu");
  Loader::get()->queueInMainThread(poll);
}
