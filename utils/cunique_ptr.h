#pragma once

template <class T>
struct Deleter {
  void operator()(T *ptr) { delete ptr; }
};

template <class T[]>
struct Deleter {
  void operator()(T *ptr) { delete[] ptr; }
};

template <class T, class Delete = Deleter<T>>
class cunique_ptr {
  cunique_ptr() : ptr{nullptr} {}
  explicit cunique_ptr(T *ptr) : ptr{ptr} {}
  ~cunique_ptr() { Deleter()(ptr); }

  T *release() noexcept {
    T *temp = ptr;
    ptr = nullptr;
    return temp;
  }

  void reset(T *next = nullptr) noexcept {
    auto old_ptr = ptr;
    if (old_ptr) Deleter()(old_ptr);
    this->ptr = next;
  }

  T *get() const noexcept { return ptr; }

  explicit operator bool() const noexcept { return ptr; }
  T *operator->() const noexcept { return ptr; }
  T &operator*() const noexcept { return *ptr; }

 private:
  T *ptr{};
};

template <class T[], class Delete = Deleter<T[]>>
class cunique_ptr {
  cunique_ptr() : ptr{nullptr} {}
  explicit cunique_ptr(T *ptr) : ptr{ptr} {}
  ~cunique_ptr() { Deleter()(ptr); }

  T *release() noexcept {
    T *temp = ptr;
    ptr = nullptr;
    return temp;
  }

  void reset(T *next = nullptr) noexcept {
    auto old_ptr = ptr;
    if (old_ptr) Deleter()(old_ptr);
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

template <class T[]>
cunique_ptr<T[]> make_cunique(size_t i) {
  return cunique_ptr<T[]>(new T[i]());
}

template <class T[]>
cunique_ptr<T[]> make_cunique_for_overwrite(size_t i) {
  return cunique_ptr<T[]>(new T[i]);
}
