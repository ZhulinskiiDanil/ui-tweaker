#pragma once

// A strong reference to a Cocos object, retained once and released once whatever is done with it.
// geode::Ref's move assignment doesn't release what it held, and geode::WeakRef's assignment repoints the
// shared controller of the old object; this one is safe in vectors that erase and in maps
#include <cocos2d.h>

#include <utility>

template <class T>
class Retained
{
public:
  Retained() = default;

  explicit Retained(T* object) : m_object(object)
  {
    if (m_object)
      m_object->retain();
  }

  Retained(Retained const& other) : Retained(other.m_object) {}

  Retained(Retained&& other) noexcept : m_object(std::exchange(other.m_object, nullptr)) {}

  // Copy and swap: the old object is released by the temporary
  Retained& operator=(Retained other) noexcept
  {
    std::swap(m_object, other.m_object);
    return *this;
  }

  ~Retained()
  {
    if (m_object)
      m_object->release();
  }

  T* get() const { return m_object; }
  T* operator->() const { return m_object; }
  explicit operator bool() const { return m_object != nullptr; }
  bool operator==(Retained const& other) const { return m_object == other.m_object; }
  bool operator==(T const* other) const { return m_object == other; }

  void reset() { *this = Retained(); }

private:
  T* m_object = nullptr;
};
