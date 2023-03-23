/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#pragma once

#include "defines.h"

#include <atomic>
#include <fstream>
#include <memory>
#include <queue>
#include <string_view>

namespace es {

class ThreadPool;

/**
 * This class represents a file buffer that reads buffer chunk of numbers in a separate thread using a thread pool.
 */
template <typename NumberType>
class BinaryFileBuffer {
  enum : std::size_t { kBuffersCount = 2 };

  /**
   * Internal representation of buffer of numbers
   */
  struct buffer_internal {
    buffer_internal() = default;

    buffer_internal(std::unique_ptr<NumberType[]> buffer) noexcept : buffer_{std::move(buffer)} {}

    buffer_internal(buffer_internal&& other) noexcept
        : is_ready_{other.is_ready_.load()}, numbers_read_{other.numbers_read_}, buffer_{std::move(other.buffer_)} {}

    friend void swap(buffer_internal& own, buffer_internal& other) noexcept {
      own.is_ready_.store(other.is_ready_.load());  // note just copy
      std::swap(own.numbers_read_, other.numbers_read_);
      std::swap(own.buffer_, other.buffer_);
    }

    std::atomic_bool is_ready_{false};      ///< Flag for indicating if buffer is ready
    std::size_t numbers_read_{0};           ///< Count of read numbers
    std::unique_ptr<NumberType[]> buffer_;  ///< Buffer
  };

 public:
  BinaryFileBuffer(std::shared_ptr<ThreadPool> pool, std::string_view file_path, std::size_t buffer_size);

  /**
   * The destructor cannot be called while executing related tasks in a thread pool.
   */
  ~BinaryFileBuffer();

  BinaryFileBuffer(BinaryFileBuffer&&) = default;

  BinaryFileBuffer& operator=(BinaryFileBuffer&&) = default;

 public:
  /**
   * Should be called before first use of get(). We need to be sure that buffer_0 has been loaded.
   */
  void waitForReady() const;

  /**
   * Gets number from buffer
   * @param number number reference
   * @return true if success otherwise false
   */
  bool get(NumberType& number);

 private:
  /**
   * Waits for readiness of a buffer in the current thread
   * @param buffer
   */
  void waitForBuffer(const buffer_internal& buffer) const;

  /**
   * Function for loading buffer_internal in parallel
   * @param stream file stream
   * @param buffer internal buffer for loading to
   * @param buffer_size size of the internal buffer
   */
  static void loadBuffer(std::ifstream& stream, buffer_internal& buffer, std::size_t buffer_size);

 private:
  std::shared_ptr<ThreadPool> thread_pool_;  ///< Thread pool for executing tasks
  std::size_t buffer_size_;                  ///< Size of the internal buffer
  std::size_t numbers_count_;                ///< Count of number corresponding to buffer_size_
  std::size_t current_index_ = 0;            ///< Current index
  std::ifstream stream_;                     ///< Input file stream

  buffer_internal buffer_0;  ///< First part of buffer
  buffer_internal buffer_1;  ///< Second part of buffer
};

}  // namespace es