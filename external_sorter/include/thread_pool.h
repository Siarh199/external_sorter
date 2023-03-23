/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#pragma once

#include "defines.h"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace es {

/**
 * Thread pool task type
 */
using task_t = std::function<void()>;

/**
 * Thread pool
 */
class ThreadPool {
  /**
   * Aliases
   */
  using mutex_type = std::mutex;
  using cv_type = std::condition_variable;

 public:
  ThreadPool();
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

 public:
  /**
   * Adds a task to the thread pool
   * @param task task
   */
  void add(task_t task);

  /**
   * Throws a stored exception (if any exists)
   * @throws
   */
  void checkException() const;

  /**
   * Checks whether pool has pending tasks
   * @return
   */
  bool hasPendingTasks() const;

  /**
   * Waits a condition in this thread
   * TODO: consider updating behavior after switching to C++20
   * @param task_flag
   */
  void waitForTask(const std::atomic_bool& task_flag) const;

 private:
  /**
   * Pops a task from the queue
   * @param task
   * @return true if the task has been extracted
   */
  bool popTask(task_t& task);

 private:
  std::atomic_uint_fast32_t active_tasks_ = 0;  ///< Amount of active tasks

  std::vector<std::thread> threads_;  ///< Working threads

  std::atomic_bool exception_flag_ = false;     ///< Exception flag
  std::exception_ptr exception_ptr_ = nullptr;  ///< Exception pointer

  mutable mutex_type mutex_;       ///< Tasks mutex
  std::atomic_bool stop_ = false;  ///< Stop flag
  cv_type cv_;                     ///< Tasks condition variable

  std::queue<task_t> tasks_;  ///< Tasks
};

}  // namespace es