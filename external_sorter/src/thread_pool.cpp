/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include "thread_pool.h"

namespace es {
namespace {

template <typename counter_type>
class ThreadCounterWatchDog {
 public:
  ThreadCounterWatchDog() = default;
  ~ThreadCounterWatchDog() noexcept {
    if (counter_ptr_) {
      counter_ptr_->fetch_sub(1, std::memory_order_release);
    }
  }

  void init(counter_type* counter) noexcept {
    counter_ptr_ = counter;

    counter_ptr_->fetch_add(1, std::memory_order_release);
  }

 private:
  counter_type* counter_ptr_ = nullptr;
};

const unsigned int kMinThreadsCount = 2;

}  // namespace

ThreadPool::ThreadPool() {
  const auto threads_count = std::max(std::thread::hardware_concurrency(), kMinThreadsCount) - 1;

  for (std::size_t i = 0; i < threads_count; ++i) {
    threads_.emplace_back([&]() {
      while (true) {
        task_t task;
        ThreadCounterWatchDog<std::atomic_uint_fast32_t> counter{};

        try {
          {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [&]() { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
              return;
            }

            std::swap(task, tasks_.front());

            tasks_.pop();

            counter.init(&active_tasks_);
          }

          task();
        } catch (...) {
          if (!exception_flag_.load()) {
            exception_ptr_ = std::current_exception();

            exception_flag_.store(true);
          }
        }
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  stop_ = true;
  cv_.notify_all();

  for (auto& thread : threads_) {
    thread.join();
  }
}

void ThreadPool::add(task_t task) {
  {
    std::scoped_lock<mutex_type> lock(mutex_);

    tasks_.push(std::move(task));
  }

  cv_.notify_one();
}

void ThreadPool::checkException() const {
  if (exception_flag_.load()) {
    std::rethrow_exception(exception_ptr_);
  }
}

bool ThreadPool::hasPendingTasks() const {
  if (active_tasks_.load() != 0) {
    return true;
  }

  std::scoped_lock<mutex_type> lock(mutex_);

  return !(tasks_.empty() && active_tasks_.load(std::memory_order_relaxed) == 0);
}

void ThreadPool::waitForTask(const std::atomic_bool& task_flag) const {
  while (!task_flag.load(std::memory_order_acquire)) {
    std::this_thread::yield();

    checkException();
  }
}

bool ThreadPool::popTask(task_t& task) {
  std::unique_lock lock(mutex_);

  cv_.wait(lock, [&]() { return stop_ || !tasks_.empty(); });

  if (stop_ && tasks_.empty()) {
    return false;
  }

  std::swap(task, tasks_.front());

  tasks_.pop();

  return true;
}

}  // namespace es