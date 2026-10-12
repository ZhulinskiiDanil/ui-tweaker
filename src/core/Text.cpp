#include "Text.hpp"

namespace
{
  // Columns are letters, not bytes: a Russian letter is two bytes of UTF-8 and one letter on the screen
  bool continues(char byte)
  {
    return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
  }

  size_t letters(std::string_view text)
  {
    size_t count = 0;
    for (char byte : text)
      if (!continues(byte))
        count++;
    return count;
  }

  // The bytes of the first `count` letters
  size_t bytesOf(std::string_view text, size_t count)
  {
    size_t at = 0;
    for (; at < text.size(); at++)
      if (!continues(text[at]) && count-- == 0)
        break;
    return at;
  }
} // namespace

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

      while (letters(word) > columns)
      {
        // Too long for any line: cut
        if (!line.empty())
        {
          lines.push_back(line);
          line.clear();
        }
        auto cut = bytesOf(word, columns);
        lines.emplace_back(word.substr(0, cut));
        word.remove_prefix(cut);
      }
      if (word.empty())
        continue;
      if (line.empty())
        line = word;
      else if (letters(line) + 1 + letters(word) <= columns)
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
  if (letters(text) <= columns)
    return std::string(text);
  if (columns <= 2)
    return std::string(text.substr(0, bytesOf(text, columns)));
  return std::string(text.substr(0, bytesOf(text, columns - 2))) + "..";
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
