#pragma once

// The undo history: changes of targets from one state to another, with undo and redo. Without Geode in it,
// the target and the state are whatever the mod keeps
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace history
{
  template <class Target, class State>
  struct Change
  {
    Target target;
    State before;
    State after;
    // What made the change (a drag, the wheel, an arrow key): only changes of one kind are merged
    int kind = 0;
    // Seconds, from any clock
    double time = 0;
    // Changes made together (a drag of several nodes): the same number, not 0. Undone and redone together
    int group = 0;
  };

  template <class Target, class State>
  class History
  {
  public:
    using Entry = Change<Target, State>;

    explicit History(size_t limit = 200) : m_limit(limit) {}

    // A change of the same kind to the same target within `mergeSeconds` of the last one becomes part of the
    // last one: a turn of the wheel is one step to undo, not twenty. Anything recorded drops the redo
    // The oldest change if there were more than the limit: dropped, it can't be undone any more
    std::optional<Entry> record(Entry change, double mergeSeconds = 0)
    {
      m_redo.clear();
      if (!m_undo.empty() && mergeSeconds > 0)
      {
        auto& last = m_undo.back();
        if (last.target == change.target && last.kind == change.kind && last.group == 0 && change.group == 0 &&
            change.time - last.time <= mergeSeconds)
        {
          last.after = std::move(change.after);
          last.time = change.time;
          return std::nullopt;
        }
      }
      m_undo.push_back(std::move(change));
      if (m_undo.size() <= m_limit)
        return std::nullopt;
      Entry dropped = std::move(m_undo.front());
      m_undo.erase(m_undo.begin());
      return dropped;
    }

    // The last change, for its `before` to be put back. It moves to the redo list
    std::optional<Entry> undo()
    {
      if (m_undo.empty())
        return std::nullopt;
      Entry change = std::move(m_undo.back());
      m_undo.pop_back();
      m_redo.push_back(change);
      return change;
    }

    // The last undone change, for its `after` to be put back
    std::optional<Entry> redo()
    {
      if (m_redo.empty())
        return std::nullopt;
      Entry change = std::move(m_redo.back());
      m_redo.pop_back();
      m_undo.push_back(change);
      return change;
    }

    // Changes the after state of `target` in the last group, if it's the last step: a run of arrow presses on several
    // nodes stays one step, as on one node
    template <class Key>
    bool amend(int group, Key const& target, State after, double time)
    {
      for (auto it = m_undo.rbegin(); it != m_undo.rend() && it->group == group && group != 0; ++it)
        if (it->target == target)
        {
          it->after = std::move(after);
          it->time = time;
          return true;
        }
      return false;
    }

    // The last change and the ones made together with it, the last first
    std::vector<Entry> undoGroup()
    {
      std::vector<Entry> changes;
      while (auto change = undo())
      {
        int group = change->group;
        changes.push_back(std::move(*change));
        if (group == 0 || m_undo.empty() || m_undo.back().group != group)
          break;
      }
      return changes;
    }

    // The last undone change and the ones made together with it
    std::vector<Entry> redoGroup()
    {
      std::vector<Entry> changes;
      while (auto change = redo())
      {
        int group = change->group;
        changes.push_back(std::move(*change));
        if (group == 0 || m_redo.empty() || m_redo.back().group != group)
          break;
      }
      return changes;
    }

    // Forgets the changes of targets that are gone, from both lists
    template <class Predicate>
    void forget(Predicate gone)
    {
      std::erase_if(m_undo, [&](Entry const& change) { return gone(change.target); });
      std::erase_if(m_redo, [&](Entry const& change) { return gone(change.target); });
    }

    void clear()
    {
      m_undo.clear();
      m_redo.clear();
    }

    std::vector<Entry> const& changes() const { return m_undo; }
    // What redo would do again
    std::vector<Entry> const& undone() const { return m_redo; }
    bool canUndo() const { return !m_undo.empty(); }
    bool canRedo() const { return !m_redo.empty(); }

  private:
    size_t m_limit;
    std::vector<Entry> m_undo;
    std::vector<Entry> m_redo;
  };
} // namespace history
