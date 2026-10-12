#include "LoggerLink.hpp"

#include "ImprovedLogger.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace
{
  constexpr char const* LOGGER_ID = "zhulis.improved-logger";

  bool linked()
  {
    return Loader::get()->isModLoaded(LOGGER_ID) && Mod::get()->getSettingValue<bool>("logger-pins");
  }
} // namespace

void logger_link::pin(std::string const& selected, size_t edited)
{
  if (!linked())
    return;
  improved_logger::watch("selected", selected);
  improved_logger::watch("edited nodes", edited);
}

void logger_link::mark(std::string const& label)
{
  if (linked())
    improved_logger::mark(label);
}

void logger_link::clear()
{
  if (Loader::get()->isModLoaded(LOGGER_ID))
    improved_logger::unwatch();
}
