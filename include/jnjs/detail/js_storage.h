#pragma once

#include <memory>

namespace jnjs::detail {

template <typename T> class stored_class {
  public:
    virtual ~stored_class() = default;

    virtual T *get() = 0;
    virtual const T *get() const = 0;
};

template <typename T> class owned_moved_class final : public stored_class<T> {
  public:
    explicit owned_moved_class(std::unique_ptr<T> v) : _v(std::move(v)) {}
    explicit owned_moved_class(T *v) : _v(v) {}

    T *get() override { return _v.get(); }
    const T *get() const override { return _v.get(); }

  private:
    std::unique_ptr<T> _v;
};

template <typename T> class owned_stored_class final : public stored_class<T> {
  public:
    explicit owned_stored_class(T v) : _v(v) {}
    template <typename... TArgs> explicit owned_stored_class(TArgs... args) : _v(std::forward<TArgs>(args)...) {}

    T *get() override { return &_v; }
    const T *get() const override { return &_v; }

  private:
    T _v;
};

template <typename T> class borrowed_stored_class final : public stored_class<T> {
  public:
    explicit borrowed_stored_class(T *v) : _v(*v) {}
    explicit borrowed_stored_class(T &v) : _v(v) {}

    T *get() override { return &_v; }
    const T *get() const override { return &_v; }

  private:
    T &_v;
};

template <typename T> class shared_class final : public stored_class<T> {
  public:
    explicit shared_class(std::shared_ptr<T> v) : _v(std::move(v)) {}

    T *get() override { return _v.get(); }
    const T *get() const override { return _v.get(); }

  private:
    std::shared_ptr<T> _v;
};

} // namespace jnjs::detail
