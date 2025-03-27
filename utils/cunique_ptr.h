#pragma once

#include <stddef.h>
#include <utility>
template <class T>
struct Deleter {
  void operator()(T *ptr) { delete ptr; }
};

template <class T>
struct Deleter<T[]> {
  void operator()(T *ptr) { delete[] ptr; }
};

template <class T, class Delete = Deleter<T>>
class cunique_ptr {
  public:
  // Default Constructor
  cunique_ptr() : ptr{nullptr} {}
  // Explicit Constructor
  explicit cunique_ptr(T *ptr) : ptr{ptr} {}
  // Destructor - overloaded () operator in Deleter
  ~cunique_ptr() { Delete()(ptr); } 

  // allows transfer of ownership to raw pointer
  T *release() noexcept {
    T *temp = ptr;
    ptr = nullptr;
    return temp;
  }

  // Allows smart pointer to point to new resource
  void reset(T *next = nullptr) noexcept {
    auto old_ptr = ptr;
    if (old_ptr) Delete()(old_ptr);
    this->ptr = next;
  }
  
  T *get() const noexcept { return ptr; }

  explicit operator bool() const noexcept { return ptr; }
  T *operator->() const noexcept { return ptr; }
  T &operator*() const noexcept { return *ptr; }

 private:
  T *ptr{};
};


template <class T>
class cunique_ptr<T[]> {
  public:
  cunique_ptr() : ptr{nullptr} {}
  explicit cunique_ptr(T *ptr) : ptr{ptr} {}
  ~cunique_ptr() { Deleter<T[]>()(ptr); }

  T *release() noexcept {
    T *temp = ptr;
    ptr = nullptr;
    return temp;
  }

  void reset(T *next = nullptr) noexcept {
    auto old_ptr = ptr;
    if (old_ptr) Deleter<T[]>{}(old_ptr);
    this->ptr = next;
  }

  T *get() const noexcept { return ptr; }

  explicit operator bool() const noexcept { return ptr; }
  T *operator->() const noexcept { return ptr; }
  T &operator*() const noexcept { return *ptr; }
  T &operator[](size_t i) const noexcept { return ptr[i]; }

 private:
  T *ptr{};
};

template <class T, class... Args>
cunique_ptr<T> make_cunique(Args &&...args) {
  return cunique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
cunique_ptr<T> make_cunique_for_overwrite() {
  return cunique_ptr<T>(new T);
}

template <class T>
cunique_ptr<T[]> make_cunique(size_t i) {
  return cunique_ptr<T[]>(new T[i]());
}

template <class T>
cunique_ptr<T[]> make_cunique_for_overwrite(size_t i) {
  return cunique_ptr<T[]>(new T[i]);
}