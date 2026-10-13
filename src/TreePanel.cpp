#include "TreePanel.hpp"

#include "Inspect.hpp"
#include "Ui.hpp"
#include "core/Text.hpp"
#include "core/Tree.hpp"

using namespace geode::prelude;

namespace
{
  constexpr size_t ROWS = 30;
  constexpr float WIDTH = 165.f;
  constexpr float ROW = 7.f;
  constexpr float MARGIN = 3.f;
  constexpr float FONT_SCALE = 0.34f;
  constexpr size_t COLUMNS = 46;
  // Deeper than this is cut: nothing in GD's menus is
  constexpr int MAX_DEPTH = 40;
  // A level in the editor has thousands of objects: the tree stops at this many rows
  constexpr size_t MAX_ROWS = 4000;
  // The rows again this often while nothing about them changes, in frames
  constexpr int REFRESH_FRAMES = 15;

  ccColor3B const WHITE = {255, 255, 255};
  ccColor3B const HIDDEN = {120, 120, 135};
  ccColor3B const SELECTED = {255, 160, 50};
  ccColor3B const HOVERED = {110, 210, 255};
  ccColor3B const MATCH = {255, 230, 140};
  ccColor3B const EDITED = {130, 230, 140};

  bool shownInTree(CCNode* node)
  {
    for (; node; node = node->getParent())
      if (!node->isVisible())
        return false;
    return true;
  }

  void collect(CCNode* node, int depth, bool hidden, bool looks, std::vector<CCNode*>& nodes, std::vector<TreeRow>& rows)
  {
    if (depth > MAX_DEPTH || nodes.size() >= MAX_ROWS)
      return;
    hidden = hidden || !node->isVisible();
    nodes.push_back(node);
    TreeRow row;
    row.depth = depth;
    row.id = std::string(node->getID());
    row.className = inspect::className(node);
    // What it shows, only while searching: a frame's name is looked up in the cache of frames
    if (looks)
      row.look = inspect::lookOf(node);
    row.hidden = hidden;
    rows.push_back(std::move(row));
    // A label's letters are sprites of their own: not worth a row each
    if (typeinfo_cast<CCLabelBMFont*>(node))
      return;
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      collect(child, depth + 1, hidden, looks, nodes, rows);
  }

  UiLabel* label(CCNode* parent, bool ascii)
  {
    auto text = UiLabel::create(FONT_SCALE, ascii);
    text->setAnchorPoint({0, 1});
    parent->addChild(text);
    return text;
  }
} // namespace

TreePanel* TreePanel::create()
{
  auto node = new TreePanel();
  if (node->init())
  {
    node->autorelease();
    return node;
  }
  delete node;
  return nullptr;
}

bool TreePanel::init()
{
  if (!CCNode::init())
    return false;
  float height = MARGIN * 2 + ROW * (ROWS + 2);
  setContentSize({WIDTH, height});
  m_background = CCLayerColor::create({15, 15, 25, 215}, WIDTH, height);
  addChild(m_background);
  m_searchBackground = CCLayerColor::create({40, 45, 70, 255}, WIDTH - MARGIN * 2, ROW);
  m_searchBackground->setPosition({MARGIN, height - MARGIN - ROW * 2});
  addChild(m_searchBackground);

  m_title = label(this, false);
  m_title->setPosition({MARGIN, height - MARGIN});
  m_title->setColor({255, 210, 90});
  m_search = label(this, false);
  m_search->setPosition({MARGIN + 1, height - MARGIN - ROW});
  for (size_t i = 0; i < ROWS; i++)
  {
    auto row = label(this, true);
    row->setPosition({MARGIN, height - MARGIN - ROW * (i + 2)});
    m_labels.push_back(row);
  }
  return true;
}

void TreePanel::refresh(CCNode* scene, CCNode* selected, CCNode* hovered, std::vector<CCNode*> const& edited)
{
  // The scene again a few times a second, or when the search changed
  if (++m_framesSinceRefresh >= REFRESH_FRAMES || m_query != m_refreshedQuery)
  {
    m_framesSinceRefresh = 0;
    m_refreshedQuery = m_query;
    m_nodes.clear();
    m_rows.clear();
    if (scene)
      collect(scene, 0, false, !m_query.empty(), m_nodes, m_rows);
    std::vector<tree::Row> rows(m_rows.begin(), m_rows.end());
    m_shown = tree::filter(rows, m_query);
  }
  auto const& rows = m_rows;

  // The selected node in view when it changes, wherever it was selected
  if (selected != m_lastSelected)
  {
    m_lastSelected = selected;
    for (size_t i = 0; i < m_shown.size(); i++)
      if (m_nodes[m_shown[i]] == selected)
        m_first = tree::scrollTo(m_first, i, ROWS);
  }
  m_first = std::min(m_first, m_shown.size() > ROWS ? m_shown.size() - ROWS : 0);

  auto const& w = ui::words();
  auto all = std::to_string(m_nodes.size()) + (m_nodes.size() >= MAX_ROWS ? "+" : "");
  m_title->setString(text::format(w.treeTitle, {std::to_string(m_shown.size()), all}));
  m_search->setString(m_query.empty() && !m_typing ? w.searchHint
                                                   : text::format(w.search, {m_query + (m_typing ? "_" : "")}));
  m_search->setColor(m_typing ? WHITE : HIDDEN);

  for (size_t i = 0; i < ROWS; i++)
  {
    auto label = m_labels[i];
    size_t at = m_first + i;
    if (at >= m_shown.size())
    {
      label->setVisible(false);
      continue;
    }
    label->setVisible(true);
    auto const& row = rows[m_shown[at]];
    auto node = m_nodes[m_shown[at]];
    bool changed = std::find(edited.begin(), edited.end(), node) != edited.end();
    std::string name = (changed ? "* " : "") + (row.id.empty() ? row.className : row.id + " " + row.className);
    auto text = text::fit(std::string(std::min(row.depth, 12) * 2, ' ') + name, COLUMNS);
    label->setString(text);
    ccColor3B color = changed ? EDITED : WHITE;
    if (!m_query.empty() && tree::matches(row, m_query))
      color = MATCH;
    if (row.hidden)
      color = HIDDEN;
    if (node == hovered)
      color = HOVERED;
    if (node == selected)
      color = SELECTED;
    label->setColor(color);
  }
}

void TreePanel::forget()
{
  m_nodes.clear();
  m_rows.clear();
  m_shown.clear();
  m_framesSinceRefresh = 1000;
  m_lastSelected = nullptr;
}

bool TreePanel::contains(CCPoint world)
{
  return isVisible() && boundingBox().containsPoint(getParent()->convertToNodeSpace(world));
}

CCNode* TreePanel::rowAt(CCPoint world, CCNode* scene)
{
  if (!contains(world))
    return nullptr;
  auto local = convertToNodeSpace(world);
  float fromTop = getContentHeight() - MARGIN - local.y;
  int row = static_cast<int>(fromTop / ROW) - 2;
  if (row < 0)
    return nullptr;
  size_t at = m_first + static_cast<size_t>(row);
  if (static_cast<size_t>(row) >= ROWS || at >= m_shown.size())
    return nullptr;
  auto node = m_nodes[m_shown[at]];
  return inspect::contains(scene, node) ? node : nullptr;
}

bool TreePanel::searchAt(CCPoint world)
{
  if (!contains(world))
    return false;
  auto local = convertToNodeSpace(world);
  float fromTop = getContentHeight() - MARGIN - local.y;
  return fromTop >= ROW && fromTop < ROW * 2;
}

void TreePanel::scroll(float notches)
{
  // Up the list for the wheel turned up
  long first = static_cast<long>(m_first) - static_cast<long>(notches * 3);
  m_first = static_cast<size_t>(std::max(0L, first));
}

void TreePanel::setTyping(bool on)
{
  m_typing = on;
}

void TreePanel::type(char c)
{
  if (m_query.size() < 40)
    m_query += c;
  m_first = 0;
}

void TreePanel::erase()
{
  if (!m_query.empty())
    m_query.pop_back();
  m_first = 0;
}

CCNode* TreePanel::firstMatch(CCNode* scene)
{
  for (auto index : m_shown)
    if (tree::matches(m_rows[index], m_query) && inspect::contains(scene, m_nodes[index]))
      return m_nodes[index];
  return nullptr;
}
