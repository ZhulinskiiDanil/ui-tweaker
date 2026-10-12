#include "../src/core/History.hpp"
#include "Check.hpp"

#include <memory>

using namespace history;

namespace
{
  using Steps = History<int, float>;

  void undoRedo()
  {
    Steps steps;
    CHECK(!steps.undo());
    steps.record({1, 0, 10});
    steps.record({2, 5, 7});
    auto last = steps.undo();
    CHECK(last && last->target == 2 && last->before == 5);
    CHECK(steps.canRedo());
    auto again = steps.redo();
    CHECK(again && again->target == 2 && again->after == 7);
    CHECK(!steps.redo());
    CHECK(steps.undo()->target == 2);
    CHECK(steps.undo()->target == 1);
    CHECK(!steps.undo());
    // Something new drops what could be redone
    steps.redo();
    steps.record({3, 1, 2});
    CHECK(!steps.canRedo());
  }

  void merging()
  {
    Steps steps;
    // Five turns of the wheel within a second: one step, from the first before to the last after
    for (int i = 0; i < 5; i++)
      steps.record({1, 1.f + i * 0.1f, 1.1f + i * 0.1f, 2, i * 0.2}, 0.5);
    CHECK(steps.changes().size() == 1);
    CHECK(near(steps.changes()[0].before, 1));
    CHECK(near(steps.changes()[0].after, 1.5f));
    // Too late, another target or another kind: new steps
    steps.record({1, 1.5f, 1.6f, 2, 5}, 0.5);
    steps.record({2, 0, 1, 2, 5.1}, 0.5);
    steps.record({2, 1, 2, 3, 5.2}, 0.5);
    CHECK(steps.changes().size() == 4);
    // Not merging without a window
    steps.record({2, 2, 3, 3, 5.2});
    CHECK(steps.changes().size() == 5);
  }

  void limits()
  {
    Steps steps(3);
    for (int i = 0; i < 5; i++)
      steps.record({i, 0, 1});
    CHECK(steps.changes().size() == 3);
    CHECK(steps.changes().front().target == 2);
  }

  void forgetting()
  {
    Steps steps;
    steps.record({1, 0, 1});
    steps.record({2, 0, 1});
    steps.record({1, 1, 2});
    steps.undo();
    steps.forget([](int target) { return target == 1; });
    CHECK(steps.changes().size() == 1);
    CHECK(steps.changes()[0].target == 2);
    CHECK(!steps.canRedo());
  }

  void ownedTargets()
  {
    // Targets that own something are moved around, not copied and lost: what the mod's retained nodes need
    History<std::shared_ptr<int>, int> steps(2);
    auto first = std::make_shared<int>(1);
    steps.record({first, 0, 1});
    steps.record({std::make_shared<int>(2), 0, 1});
    steps.record({std::make_shared<int>(3), 0, 1});
    // The first one dropped off the end
    CHECK(first.use_count() == 1);
    steps.clear();
    CHECK(!steps.canUndo());
  }
} // namespace

int main()
{
  undoRedo();
  merging();
  limits();
  forgetting();
  ownedTargets();
  return finish("history");
}
