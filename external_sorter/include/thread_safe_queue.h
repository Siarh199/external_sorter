/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#pragma once

#include <functional>
#include <mutex>
#include <type_traits>

namespace es {

/**
 * Class for providing thread safe queue functionality
 * @tparam Container
 */
template <class Container>
class ThreadSafeQueue {
 public:
  /**
   * Type aliases
   */
  using container_type = Container;
  using value_type = typename Container::value_type;
  using reference = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using size_type = typename Container::size_type;

 private:
  /**
   * Type aliases
   */
  using mutex_type = std::mutex;
  using lock_type = std::scoped_lock<mutex_type>;

 public:
  ThreadSafeQueue() noexcept(std::is_nothrow_default_constructible_v<Container>) = default;

 public:
  /**
   * Checks whether the queue is empty or not
   * @return true if the queue is empty, otherwise false
   */
  bool empty() const { return callWithLock(container_, mutex_, &container_type::empty); }

  /**
   * Returns current size of the queue
   * @return
   */
  size_type size() const { return callWithLock(container_, mutex_, &container_type::size); }

 public:
  /**
   * Pushes a value to the queue
   * @param value value
   */
  void push(value_type&& value) {
    using push_rvalue_ptr_t = void (container_type::*)(typename container_type::value_type &&);

    return callWithLock(container_, mutex_, push_rvalue_ptr_t{&container_type::push}, std::forward<value_type>(value));
  }

  /**
   * Emplace a value to the queue
   * @param value value
   */
  template <class... Args>
  decltype(auto) emplace(Args&&... args) {
    using emplace_ptr_t = reference (container_type::*)(Args && ...);
    return callWithLock(container_, mutex_, emplace_ptr_t{&container_type::emplace}, std::forward<Args>(args)...);
  }

  /**
   * Gets and pops the first from the queue
   * @param value
   * @return true if success, otherwise false
   */
  bool pop(value_type& value) {
    lock_type lock(mutex_);

    if (container_.empty()) {
      return false;
    }

    std::swap(value, container_.front());

    container_.pop();

    return true;
  }

 private:
  /**
   * Helper function for calling methods of underlying container with lock
   * @tparam Fun function type
   * @tparam Args arguments types
   * @param container underlying container
   * @param mutex mutex
   * @param func function to call
   * @param args arguments for func
   * @return result of func
   */
  template <class Fun, class... Args>
  static decltype(auto) callWithLock(container_type& container, mutex_type& mutex, Fun&& func, Args&&... args) {
    lock_type lock(mutex);

    return std::invoke(std::forward<Fun>(func), std::forward<container_type>(container), std::forward<Args>(args)...);
  }

 private:
  container_type container_;  ///< queue container
  mutable mutex_type mutex_;  ///< mutex
};

}  // namespace es