#include "Text.hpp"

std::vector<std::string> text::wrap(std::string_view text, size_t columns)
{
  std::vector<std::string> lines;
  if (columns == 0)
    return lines;

  size_t start = 0;
  while (start <= text.size())
  {
    auto end = text.find('\n', start);
    auto paragraph = text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);

    std::string line;
    size_t at = 0;
    while (at < paragraph.size())
    {
      auto space = paragraph.find(' ', at);
      auto word = paragraph.substr(at, space == std::string_view::npos ? std::string_view::npos : space - at);
      at = space == std::string_view::npos ? paragraph.size() : space + 1;

      while (word.size() > columns)
      {
        // Too long for any line: cut
        if (!line.empty())
        {
          lines.push_back(line);
          line.clear();
        }
        lines.emplace_back(word.substr(0, columns));
        word.remove_prefix(columns);
      }
      if (word.empty())
        continue;
      if (line.empty())
        line = word;
      else if (line.size() + 1 + word.size() <= columns)
        line += " " + std::string(word);
      else
      {
        lines.push_back(line);
        line = word;
      }
    }
    lines.push_back(line);

    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  return lines;
}

std::string text::fit(std::string_view text, size_t columns)
{
  if (text.size() <= columns)
    return std::string(text);
  // Not inside a letter of more than a byte: back to where one starts
  auto cut = [&](size_t at)
  {
    while (at > 0 && at < text.size() && (static_cast<unsigned char>(text[at]) & 0xC0) == 0x80)
      at--;
    return std::string(text.substr(0, at));
  };
  if (columns <= 2)
    return cut(columns);
  return cut(columns - 2) + "..";
}

std::string text::format(std::string_view pattern, std::initializer_list<std::string> values)
{
  std::string out;
  auto value = values.begin();
  size_t at = 0;
  while (at < pattern.size())
  {
    auto hole = pattern.find("{}", at);
    if (hole == std::string_view::npos || value == values.end())
    {
      out += pattern.substr(at);
      break;
    }
    out += pattern.substr(at, hole - at);
    out += *value++;
    at = hole + 2;
  }
  return out;
}

size_t text::holes(std::string_view pattern)
{
  size_t count = 0;
  for (auto at = pattern.find("{}"); at != std::string_view::npos; at = pattern.find("{}", at + 2))
    count++;
  return count;
}
