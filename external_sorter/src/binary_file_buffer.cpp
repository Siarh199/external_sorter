/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include "binary_file_buffer.h"

#include "thread_pool.h"
#include "utils.h"

#include <thread>

namespace es {

template <typename NumberType>
BinaryFileBuffer<NumberType>::BinaryFileBuffer(std::shared_ptr<ThreadPool> pool, std::string_view file_path,
                                               std::size_t buffer_size)
    : thread_pool_{std::move(pool)},
      buffer_size_{RoundSize<NumberType>(buffer_size / kBuffersCount)},
      numbers_count_{buffer_size_ / sizeof(NumberType)},
      stream_{OpenInputBinaryFileStream(file_path)},
      buffer_0{std::make_unique<NumberType[]>(numbers_count_)},
      buffer_1{std::make_unique<NumberType[]>(numbers_count_)} {
  thread_pool_->add([this]() {
    loadBuffer(stream_, buffer_0, buffer_size_);

    std::this_thread::yield();

    loadBuffer(stream_, buffer_1, buffer_size_);
  });
}

template <typename NumberType>
BinaryFileBuffer<NumberType>::~BinaryFileBuffer() = default;

template <typename NumberType>
void BinaryFileBuffer<NumberType>::waitForReady() const {
  waitForBuffer(buffer_0);
}

template <typename NumberType>
bool BinaryFileBuffer<NumberType>::get(NumberType& number) {
  if (current_index_ < buffer_0.numbers_read_) {
    number = buffer_0.buffer_[current_index_++];

    return true;
  } else if (buffer_0.numbers_read_ > 0) {
    waitForBuffer(buffer_1);

    swap(buffer_0, buffer_1);

    current_index_ = 0;

    buffer_1.is_ready_.store(false);
    buffer_1.numbers_read_ = 0;

    thread_pool_->add([this]() { loadBuffer(stream_, buffer_1, buffer_size_); });

    return get(number);
  }

  return false;
}

template <typename NumberType>
void BinaryFileBuffer<NumberType>::waitForBuffer(const BinaryFileBuffer<NumberType>::buffer_internal& buffer) const {
  thread_pool_->waitForTask(buffer.is_ready_);
}

template <typename NumberType>
void BinaryFileBuffer<NumberType>::loadBuffer(std::ifstream& stream,
                                              BinaryFileBuffer<NumberType>::buffer_internal& buffer,
                                              std::size_t buffer_size) {
  auto [ok, bytes_read] = ReadFileStream(stream, reinterpret_cast<char*>(buffer.buffer_.get()), buffer_size);

  if (!ok) {
    throw MakeException("Failed to read file: ", errno);
  }

  buffer.numbers_read_ = bytes_read / sizeof(NumberType);

  buffer.is_ready_.store(true, std::memory_order_release);
}

template class BinaryFileBuffer<number_t>;

}  // namespace es
