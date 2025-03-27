#pragma once

#include <cstring>
#include <iostream>
#include <string>

class cstring_view {
 public:
  const static size_t npos = -1;

  explicit cstring_view() = default;
  explicit cstring_view(nullptr_t) = delete;
  cstring_view(const std::string &s)
      : first{s.data()}, last{s.data() + s.size()} {}
  explicit cstring_view(const char *c, size_t l) : first{c}, last{c + l} {}
  explicit cstring_view(const char *c) : first{c}, last{c + strlen(c)} {}

  template <class It, class End>
  cstring_view(It first, End last) : first{&*first}, last{&*last} {}

  const char *begin() const { return first; }
  const char *cbegin() const { return first; }

  const char *end() const { return last; }
  const char *cend() const { return last; }

  const char &operator[](size_t pos) const { return first[pos]; }
  const char &front() const { return *first; }
  const char &back() const { return *(last - 1); }
  const char *data() const { return first; }

  size_t size() const { return last - first; }
  bool empty() const { return first == last; }

  void remove_prefix(size_t n) { first += n; }
  void remove_suffix(size_t n) { last -= n; }

  bool starts_with(cstring_view other) const {
    return (size() >= other.size()) && (substr(0, other.size()) == other);
  }

  cstring_view substr(size_t pos, size_t count = npos) const {
    if (count == npos || pos + count > size())
      return cstring_view{first + pos, last};
    else
      return cstring_view{first + pos, first + pos + count};
  }

  bool operator==(const cstring_view &other) const {
    if (size() != other.size()) return false;
    for (size_t i = 0; i < size(); ++i) {
      if (first[i] != other[i]) return false;
    }
    return true;
  }

  bool operator==(const char *c) const { return *this == cstring_view{c}; }

  bool operator==(const std::string &s) const {
    return *this == cstring_view{s};
  }

  size_t find(cstring_view sv, size_t pos = 0) const {
    if (size() < sv.size()) return npos;  // avoid underflow
    for (size_t i = 0; i <= size() - sv.size(); ++i) {
      if (substr(i, sv.size()) == sv) return i;
    }
    return npos;
  }

  size_t find(char c, size_t pos = 0) const {
    return find(cstring_view{static_cast<const char *>(&c), size_t{1}});
  }

  size_t find(const char *s, size_t pos, size_t count) const {
    return find(cstring_view{s, count}, pos);
  }

  size_t find(const char *s, size_t pos) const {
    return find(cstring_view{s}, pos);
  }

  size_t rfind(cstring_view sv, size_t pos = npos) const {
    if (empty() || size() < sv.size()) return npos;  // avoid underflow
    for (size_t i = std::min(pos, size()); i >= 1; --i) {
      if (substr(i - 1, sv.size()) == sv) return i - 1;
    }
    return npos;
  }

  size_t rfind(char c, size_t pos = npos) const {
    return rfind(cstring_view{static_cast<const char *>(&c), size_t{1}});
  }

  size_t rfind(const char *s, size_t pos, size_t count) const {
    return rfind(cstring_view{s, count}, pos);
  }

  size_t rfind(const char *s, size_t pos = npos) const {
    return rfind(cstring_view{s}, pos);
  }

  friend std::ostream &operator<<(std::ostream &os, const cstring_view &sv) {
    return os.write(sv.begin(), sv.size());
  }

 private:
  const char *first{};
  const char *last{};
};
