#include "Tree.hpp"

#include <algorithm>
#include <cctype>

namespace
{
  std::string lower(std::string_view text)
  {
    std::string out(text);
    for (auto& c : out)
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
  }
} // namespace

bool tree::matches(Row const& row, std::string_view query)
{
  auto id = lower(row.id);
  auto className = lower(row.className);
  auto look = lower(row.look);
  auto words = lower(query);
  size_t at = 0;
  while (at < words.size())
  {
    auto space = words.find(' ', at);
    auto word = words.substr(at, space == std::string::npos ? std::string::npos : space - at);
    at = space == std::string::npos ? words.size() : space + 1;
    if (word.empty())
      continue;
    if (id.find(word) == std::string::npos && className.find(word) == std::string::npos &&
        look.find(word) == std::string::npos)
      return false;
  }
  return true;
}

std::vector<size_t> tree::filter(std::vector<Row> const& rows, std::string_view query)
{
  std::vector<size_t> out;
  bool empty = std::all_of(query.begin(), query.end(), [](char c) { return c == ' '; });
  if (empty)
  {
    for (size_t i = 0; i < rows.size(); i++)
      out.push_back(i);
    return out;
  }

  std::vector<bool> keep(rows.size(), false);
  for (size_t i = 0; i < rows.size(); i++)
  {
    if (!matches(rows[i], query))
      continue;
    keep[i] = true;
    // Up the tree: the closest row before it with a smaller depth, again and again
    int depth = rows[i].depth;
    for (size_t j = i; j-- > 0 && depth > 0;)
      if (rows[j].depth < depth)
      {
        keep[j] = true;
        depth = rows[j].depth;
      }
  }
  for (size_t i = 0; i < rows.size(); i++)
    if (keep[i])
      out.push_back(i);
  return out;
}

std::optional<char> tree::typed(int key, bool shift)
{
  if (key >= 'A' && key <= 'Z')
    return static_cast<char>(key - 'A' + 'a');
  if (key >= '0' && key <= '9' && !shift)
    return static_cast<char>(key);
  // Numpad digits
  if (key >= 0x60 && key <= 0x69)
    return static_cast<char>('0' + key - 0x60);
  switch (key)
  {
  case 0xBD: // VK_OEM_MINUS
    return shift ? '_' : '-';
  case 0xBE: // VK_OEM_PERIOD
    return '.';
  case 0xBF:   // VK_OEM_2
  case 0x1007: // Geode's KEY_Slash, what it makes of VK_OEM_2
    return '/';
  case 0x20:
    return ' ';
  default:
    return std::nullopt;
  }
}

size_t tree::scrollTo(size_t first, size_t row, size_t visible)
{
  if (visible == 0)
    return row;
  if (row < first)
    return row;
  if (row >= first + visible)
    return row - visible + 1;
  return first;
}
