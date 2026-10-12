#include "SelfTest.hpp"

#include "Editor.hpp"
#include "Inspect.hpp"
#include "Retained.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/CreatorLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include <cmath>
#include <chrono>
#include <cstdlib>
#include <functional>

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
  void edgeCases();

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

  // Ctrl+arrows move the anchor point and keep the node where it is; [ and ] change the z-order. Both undo
  void anchorAndOrder(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto quad = inspect::worldQuad(node);
    editor::select(node);
    int direction = before.anchor.x > 0.25f ? -1 : 1;
    editor::stepAnchor(direction, direction);
    auto moved = inspect::stateOf(node);
    check(!(moved.anchor == before.anchor), fmt::format("Ctrl+arrows moved the anchor point of {}", name));
    auto after = inspect::worldQuad(node);
    bool still = true;
    for (size_t i = 0; i < 4; i++)
      still = still && close(after.corners[i].x, quad.corners[i].x) && close(after.corners[i].y, quad.corners[i].y);
    check(still, fmt::format("{} stayed where it was with its new anchor point", name));
    auto code = editor::codeFor(node);
    check(code.find("setAnchorPoint") != std::string::npos, fmt::format("the code for {} sets the anchor point", name));
    auto problem = inspect::checkCode(node, code);
    check(problem.empty(), fmt::format("the code with the new anchor point puts {} where it is: {}", name, problem));

    editor::stepZOrder(1);
    check(node->getZOrder() == before.zOrder + 1, fmt::format("] raised the z-order of {}", name));
    check(editor::codeFor(node).find("setZOrder") != std::string::npos, "the code sets the z-order");
    check(editor::undo() && editor::undo(), "the anchor and the z-order are two steps to undo");
    check(inspect::stateOf(node) == before, fmt::format("undo put back the anchor and the z-order of {}", name));
  }

  // Enter, then x, y and the scale typed with the keys a user would press, expressions included
  void typing(CCNode* node)
  {
    auto name = inspect::name(node);
    auto before = inspect::stateOf(node);
    auto win = CCDirector::get()->getWinSize();
    editor::select(node);
    editor::startTyping();
    check(editor::typing(), "Enter starts typing values");
    // w/2+40 Tab h-25 Tab 1.5 Enter
    std::vector<std::pair<int, bool>> keys = {
        {'W', false}, {0xBF, false}, {'2', false}, {0xBB, true}, {'4', false}, {'0', false}, {KEY_Tab, false},
        {'H', false}, {0xBD, false}, {'2', false}, {'5', false}, {KEY_Tab, false},
        {'1', false}, {0xBE, false}, {'5', false}, {KEY_Enter, false}};
    for (auto [key, shift] : keys)
      editor::typingKey(key, shift);
    auto after = inspect::stateOf(node);
    check(!editor::typing(), "Enter applied the values");
    check(close(after.position.x, win.width / 2 + 40) && close(after.position.y, win.height - 25),
          fmt::format("typed w/2+40, h-25 put {} at ({}, {})", name, after.position.x, after.position.y));
    check(close(after.scaleX, 1.5f), fmt::format("typed 1.5 scaled {} to {}", name, after.scaleX));
    auto problem = inspect::checkCode(node, editor::codeFor(node));
    check(problem.empty(), fmt::format("the code for the typed values puts {} where it is: {}", name, problem));

    // Something that can't be read stays to be fixed; Esc lets it go
    editor::startTyping();
    for (auto [key, shift] : std::vector<std::pair<int, bool>>{{'P', false}, {0xBD, false}, {KEY_Enter, false}})
      editor::typingKey(key, shift);
    check(editor::typing(), "a value that can't be read isn't applied");
    editor::typingKey(KEY_Escape, false);
    check(!editor::typing() && inspect::stateOf(node) == after, "Esc lets the values go");

    check(editor::undo(), "the typed values are a step to undo");
    check(inspect::stateOf(node) == before, fmt::format("undo put {} back after the typed values", name));
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
    {
      snapping(shown[0]);
      anchorAndOrder(shown[0]);
      typing(shown[0]);
    }

    edgeCases();
  }

  // For the screenshot: the Geode button selected, the mouse over a visible sibling of it (the panel covers the
  // top left) or over a label. Whether there's something to measure the distance to
  bool prepareScreenshot()
  {
    auto scene = CCDirector::get()->getRunningScene();
    std::vector<CCNode*> shown;
    if (auto node = scene->getChildByIDRecursive("geode.loader/geode-button"); node && visibleInTree(node))
      shown.push_back(node);
    if (auto label = findLabel(scene))
      shown.push_back(label);
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
    return other != nullptr;
  }

  // ! --- Over several frames --- !

  // Steps run one after another, each followed by its number of frames for the game to go on
  constexpr int CROWD = 5000;

  double now()
  {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  }

  struct Step
  {
    int frames;
    std::function<void()> run;
  };

  std::vector<Step> g_steps;
  size_t g_next = 0;
  int g_wait = 0;

  void tick()
  {
    if (g_wait > 0)
    {
      g_wait--;
      Loader::get()->queueInMainThread(tick);
      return;
    }
    if (g_next >= g_steps.size())
      return;
    auto step = g_steps[g_next++];
    step.run();
    g_wait = step.frames;
    Loader::get()->queueInMainThread(tick);
  }

  // The first main level, the edit mode on in it: a label of the level's interface dragged, its code checked,
  // undone. The level plays on meanwhile, the player dies and starts again
  void playLevel()
  {
    g_steps.push_back({180,
                       []
                       {
                         auto level = GameLevelManager::get()->getMainLevel(1, false);
                         CCDirector::get()->replaceScene(PlayLayer::scene(level, false, false));
                       }});
    g_steps.push_back({30,
                       []
                       {
                         auto scene = CCDirector::get()->getRunningScene();
                         check(scene && scene->getChildByType<PlayLayer>(0), "the level is playing");
                         check(editor::active(), "the edit mode is on in the level");
                         auto label = findLabel(scene);
                         check(label != nullptr, "a label is in the level");
                         if (!label)
                           return;
                         Case item{Retained<CCNode>(label), {}, {}};
                         edit(item);
                         int undone = 0;
                         while (editor::undo())
                           undone++;
                         check(inspect::stateOf(label) == item.before, "undo put the level's label back");
                         editor::select(label);
                         editor::pinMouse(std::nullopt);
                         editor::setMeasuring(std::nullopt);
                       }});
    g_steps.push_back({1,
                       []
                       {
                         g_frames = 0;
                         finish(false);
                       }});
  }

  // A node removed while it's selected, hovered, in the history and in the tree; then the scene replaced while
  // a node is selected, an edit in the new scene, and back to the main menu
  void edgeCases()
  {
    static Retained<CCNode> temp;
    static CCPoint middle;
    static Retained<CCNode> crowd;
    static double frameStart = 0;
    static std::vector<double> frameTimes;
    g_steps = {
        // A crowded scene, like a level in the editor: 5000 sprites. What the hover and the tree cost then
        {1,
         []
         {
           auto win = CCDirector::get()->getWinSize();
           auto node = CCNode::create();
           node->setID("selftest-crowd"_spr);
           for (int i = 0; i < CROWD; i++)
           {
             auto sprite = CCSprite::createWithSpriteFrameName("square_01_001.png");
             if (!sprite)
               sprite = CCSprite::create();
             sprite->setPosition({static_cast<float>(i % 100) * win.width / 100, static_cast<float>(i / 100) * 5});
             sprite->setScale(0.2f);
             node->addChild(sprite);
           }
           CCDirector::get()->getRunningScene()->addChild(node, -1);
           crowd = Retained<CCNode>(node);

           auto start = now();
           size_t hits = 0;
           for (int i = 0; i < 10; i++)
             hits += inspect::hitsAt(CCDirector::get()->getRunningScene(), {win.width / 2, 20}).size();
           auto millis = (now() - start) * 100;
           log::info("Self test: what's under the mouse among {} more nodes: {:.2f} ms ({} hits)", CROWD, millis, hits / 10);
           check(millis < 30, fmt::format("the hover in a crowded scene takes {:.2f} ms", millis));
           editor::setTreeVisible(true);
           frameTimes.clear();
           frameStart = now();
         }},
        {30,
         []
         {
           // The frames with the tree open over the crowd, the edit mode drawing as it does
           auto millis = (now() - frameStart) * 1000 / 30;
           log::info("Self test: a frame with the tree open over {} more nodes: {:.2f} ms on average", CROWD, millis);
           check(millis < 50, fmt::format("a frame with the tree open in a crowded scene takes {:.2f} ms", millis));
           editor::setTreeVisible(false);
           crowd->removeFromParent();
           crowd.reset();
         }},
        {3,
         []
         {
           auto win = CCDirector::get()->getWinSize();
           middle = ccp(win.width / 2, win.height / 2);
           CCNode* node = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
           if (!node)
             node = CCLayerColor::create({255, 255, 255, 255}, 20, 20);
           node->setID("selftest-node"_spr);
           node->setPosition(middle);
           CCDirector::get()->getRunningScene()->addChild(node, 1000);
           temp = Retained<CCNode>(node);
           editor::setTreeVisible(true);
           editor::grab(node, middle);
           editor::dragTo(middle + ccp(15, 10));
           editor::release();
           editor::wheel(1, false);
           auto center = inspect::worldQuad(node).center();
           editor::pinMouse(CCPoint{center.x, center.y});
           check(editor::selected() == node, "the new node is selected");
         }},
        {3,
         []
         {
           // Gone from the scene while selected, hovered, in the history and in the tree's rows
           temp->removeFromParent();
           temp.reset();
         }},
        {1,
         []
         {
           check(editor::selected() == nullptr, "a removed node is unselected");
           check(!editor::undo(), "undo skips a removed node's steps");
           check(editor::editedNodes().empty(), "a removed node isn't among the edited ones");
           editor::setTreeVisible(false);
           editor::pinMouse(middle);
           log::info("Self test: a node removed while selected: no crash");
           auto button = CCDirector::get()->getRunningScene()->getChildByIDRecursive("geode.loader/geode-button");
           editor::select(button);
           auto scene = CreatorLayer::scene();
           CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, scene));
         }},
        {5,
         []
         {
           check(editor::active(), "the edit mode is on during the transition");
           check(editor::selected() == nullptr, "the old scene's node is unselected in the transition");
         }},
        {60,
         []
         {
           auto scene = CCDirector::get()->getRunningScene();
           check(scene && scene->getChildByType<CreatorLayer>(0), "the creator layer is there");
           auto picked = editor::pressAt(middle);
           if (picked)
           {
             editor::dragTo(middle + ccp(20, 0));
             editor::release();
           }
           check(picked != nullptr, "something is picked in the new scene");
           check(editor::historySize() >= 1, "an edit in the new scene is in the history");
           log::info("Self test: picked {} in the creator layer", picked ? inspect::name(picked) : "nothing");
           CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, MenuLayer::scene(false)));
         }},
        {120,
         []
         {
           check(editor::active(), "the edit mode is on after two scene changes");
           check(editor::historySize() == 0, "the history forgot the creator layer's nodes");
           if (std::string(std::getenv("UI_TWEAKER_SELFTEST")) == "play")
           {
             playLevel();
             return;
           }
           auto measurable = prepareScreenshot();
           g_frames = 0;
           finish(measurable);
         }},
    };
    g_next = 0;
    g_wait = 0;
    tick();
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
