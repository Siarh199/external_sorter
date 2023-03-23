/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include "external_sorter.h"

#include "binary_file_buffer.h"
#include "thread_pool.h"
#include "thread_safe_queue.h"
#include "utils.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <queue>
#include <string_view>
#include <thread>
#include <vector>

namespace es {

namespace {

const std::string_view kOutputFileName{"output"};
const std::string_view kIntermediateDirectoryName{"intermediate"};
const std::string_view kIntermediateFileName{"chunk_"};

/**
 * Calculates amount of memory which can be used for buffers. It is not possible to use all available memory because we
 * need take into account size of thread pool, stacks, and objects.
 * @param total_size total memory size
 * @return size
 */
std::size_t CalcUsefulMemorySize(std::size_t total_size) noexcept {
  // magic numbers which were found with perf
  constexpr std::size_t num = 9;
  constexpr std::size_t denom = 16;

  return total_size / denom * num;
}

std::string CreateOutputFilePath(std::string output_directory_path) {
  output_directory_path.append(kOutputFileName);

  return output_directory_path;
}

std::filesystem::path CreateIntermediateDirectoryPath(std::string_view output_directory_path) {
  std::filesystem::path intermediate_path{output_directory_path};
  intermediate_path /= kIntermediateDirectoryName;

  return intermediate_path;
}

std::filesystem::path CreateIntermediateFilePath(std::filesystem::path intermediate_path, uint32_t id) {
  intermediate_path /= kIntermediateFileName;
  intermediate_path += std::to_string(id);

  return intermediate_path;
}

void WriteFile(std::ofstream& stream, const char* buffer, std::size_t size, std::string_view path) {
  stream.write(buffer, size);
  if (!stream) {
    throw MakeException("Failed to write the file ", path, std::string_view{": "}, errno);
  }
}

void WriteIntermediateFile(std::filesystem::path intermediate_directory_path, uint32_t id, const char* buffer,
                           std::size_t size) {
  auto path = CreateIntermediateFilePath(intermediate_directory_path, id++).string();
  auto stream = OpenOutputBinaryFileStream(path);

  WriteFile(stream, buffer, size, path);
}

/**
 * Calculates amount of memory which will be used for intermediate files buffers
 * @param total_memory total amount of memory
 * @return size
 */
std::size_t CalcFilesBuffersMemorySize(std::size_t total_memory) {
  // magic numbers which were found with perf
  constexpr std::size_t num = 3;
  constexpr std::size_t denom = 4;

  return total_memory * num / denom;
}

/**
 * Creates buffers for intermediate files
 * @tparam NumberType
 * @param thread_pool thread pool
 * @param files_count amount of intermediate files
 * @param file_buffer_memory_size size of a buffer
 * @param intermediate_directory_path path to intermediate directory
 * @return buffers
 */
template <typename NumberType>
std::vector<BinaryFileBuffer<NumberType>> CreateIntermediateFilesBuffers(
    std::shared_ptr<ThreadPool> thread_pool, std::size_t files_count, std::size_t file_buffer_memory_size,
    const std::filesystem::path& intermediate_directory_path) {
  std::vector<BinaryFileBuffer<NumberType>> files_buffers;
  files_buffers.reserve(files_count);

  for (uint32_t i = 0; i < files_count; ++i) {
    files_buffers.emplace_back(thread_pool, CreateIntermediateFilePath(intermediate_directory_path, i).string(),
                               file_buffer_memory_size);
  }

  return files_buffers;
}

/**
 * Class for keeping information about an intermediate chunk
 * @tparam NumberType
 */
template <typename NumberType>
struct MergeData {
  std::size_t file_buffer_index_;  ///< index of an intermediate file buffer
  NumberType value_;               ///< The last number from the intermediate file buffer
};

/**
 * Greater functor
 * @tparam NumberType
 */
template <typename NumberType>
struct MergeDataComparator {
  bool operator()(const MergeData<NumberType>& lv, const MergeData<NumberType>& rv) noexcept {
    return lv.value_ > rv.value_;
  }
};

const std::size_t kMinAvailableMemory = 2 * 1024 * 1024;

template <typename NumberType>
struct MergeBuffer {
  std::atomic_bool is_ready_to_fill_ = true;
  std::unique_ptr<NumberType[]> buffer_;
};

exception_t MakeFailedReadFileException(std::string_view file_path, int err) {
  return MakeException("Failed to read ", file_path, std::string_view{": "}, err);
}

}  // namespace

template <typename NumberType>
ExternalSorter<NumberType>::ExternalSorter(std::size_t available_memory, std::string input_file_path,
                                           std::string output_directory_path, std::shared_ptr<ThreadPool> thread_pool)
    : available_memory_{RoundSize<NumberType>(CalcUsefulMemorySize(available_memory))},
      input_file_path_{input_file_path},
      output_directory_path_{std::move(output_directory_path)},
      output_file_path_{CreateOutputFilePath(output_directory_path_)},
      intermediate_directory_path_{CreateIntermediateDirectoryPath(output_directory_path_)},
      input_file_stream_{OpenInputBinaryFileStream(input_file_path_)},
      output_file_stream_{OpenOutputBinaryFileStream(output_file_path_)},
      thread_pool_{std::move(thread_pool)} {
  if (available_memory_ < kMinAvailableMemory) {
    throw MakeException("There is not enough memory.");
  }
}

template <typename NumberType>
ExternalSorter<NumberType>::~ExternalSorter() = default;

template <typename NumberType>
void ExternalSorter<NumberType>::sort() {
  createIntermediateDirectory();

  createSortedChunksImplMultiThreaded();

  mergeSortedChunksImpl();
}

// Sorting with this method performs worse than with createSortedChunksImplMultiThreaded().
template <typename NumberType>
void ExternalSorter<NumberType>::createSortedChunksImplSingleThreaded() {
  auto numbers_count = available_memory_ / sizeof(NumberType);
  auto buffer = std::make_unique<NumberType[]>(numbers_count);

  while (true) {
    auto [ok, bytes_read] =
        ReadFileStream(input_file_stream_, reinterpret_cast<char*>(buffer.get()), available_memory_);

    if (!ok) {
      throw MakeFailedReadFileException(input_file_path_, errno);
    }

    if (bytes_read != 0) {
      std::stable_sort(buffer.get(), buffer.get() + bytes_read / sizeof(NumberType));

      WriteIntermediateFile(intermediate_directory_path_, intermediate_files_count_++,
                            reinterpret_cast<char*>(buffer.get()), bytes_read);
    }

    if (available_memory_ != bytes_read) {
      break;
    }
  }
}

template <typename NumberType>
void ExternalSorter<NumberType>::createIntermediateDirectory() const {
  std::error_code ec{};
  std::filesystem::create_directory(intermediate_directory_path_, ec);
  if (ec) {
    throw MakeException("Failed to create the intermediate directory: ", ec);
  }
}

template <typename NumberType>
void ExternalSorter<NumberType>::createSortedChunksImplMultiThreaded() {
  const std::size_t chunks_count = std::thread::hardware_concurrency();
  const auto chunk_numbers_count = (available_memory_ / sizeof(NumberType)) / chunks_count;

  using number_buffer_t = std::unique_ptr<NumberType[]>;
  auto chunks_queue = std::make_shared<ThreadSafeQueue<std::queue<number_buffer_t>>>();

  for (std::size_t i = 0; i < chunks_count; ++i) {
    chunks_queue->push(std::make_unique<NumberType[]>(chunk_numbers_count));
  }

  while (true) {
    number_buffer_t buffer;

    if (!chunks_queue->pop(buffer)) {
      std::this_thread::yield();

      thread_pool_->checkException();

      continue;
    }

    thread_pool_->checkException();

    auto [ok, bytes_read] = ReadFileStream(input_file_stream_, reinterpret_cast<char*>(buffer.get()),
                                           chunk_numbers_count * sizeof(NumberType));

    if (!ok) {
      throw MakeFailedReadFileException(input_file_path_, errno);
    }

    if (bytes_read == 0) {
      break;
    }

    // Sorts chunk and writes it to a file in a separate thread.
    thread_pool_->add([this, chunks_queue, buff = std::make_shared<number_buffer_t>(std::move(buffer)),
                       bytes_read = bytes_read]() mutable {
      NumberType* buffer{(*buff).get()};

      std::stable_sort(buffer, buffer + bytes_read / sizeof(NumberType));

      WriteIntermediateFile(intermediate_directory_path_, intermediate_files_count_++, reinterpret_cast<char*>(buffer),
                            bytes_read);

      chunks_queue->push(std::move(*buff));
    });
  }

  while (thread_pool_->hasPendingTasks()) {
    std::this_thread::yield();
  }

  thread_pool_->checkException();
}

template <typename NumberType>
void ExternalSorter<NumberType>::mergeSortedChunksImpl() {
  const std::size_t files_count = intermediate_files_count_.load();

  if (files_count == 0) {
    return;
  }

  const std::size_t file_buffer_memory_size =
      RoundSize<NumberType>(CalcFilesBuffersMemorySize(available_memory_) / files_count);

  auto files_buffers = CreateIntermediateFilesBuffers<NumberType>(thread_pool_, files_count, file_buffer_memory_size,
                                                                  intermediate_directory_path_);

  constexpr std::size_t merge_buffers_count = 2;
  const auto merge_buffer_size_in_bytes =
      RoundSize<NumberType>((available_memory_ - file_buffer_memory_size) / merge_buffers_count);
  const auto merge_numbers_count = merge_buffer_size_in_bytes / sizeof(NumberType);

  std::size_t current_merge_buffer_index = 0;

  // Write the first buffer to a file in a separate thread while filling the second buffer in the current thread.
  MergeBuffer<NumberType> merge_buffer_0{true, std::make_unique<NumberType[]>(merge_numbers_count)};
  MergeBuffer<NumberType> merge_buffer_1{true, std::make_unique<NumberType[]>(merge_numbers_count)};

  // min heap
  std::priority_queue<MergeData<NumberType>, std::vector<MergeData<NumberType>>, MergeDataComparator<NumberType>>
      merge_queue;

  // initialization of all buffers and merge queue
  for (std::size_t i = 0; i < std::size(files_buffers); ++i) {
    files_buffers[i].waitForReady();

    NumberType number;
    if (!files_buffers[i].get(number)) {
      continue;
    }

    merge_queue.push(MergeData<NumberType>{i, number});
  }

  if (merge_queue.empty()) {
    return;
  }

  // swap buffers and write buffer in a separate thread
  auto swapMergeBufferAndCreateTask = [this](auto& merge_buffer_0, auto& merge_buffer_1,
                                             auto& current_merge_buffer_index, const auto merge_buffer_size_in_bytes) {
    thread_pool_->waitForTask(merge_buffer_1.is_ready_to_fill_);

    std::swap(merge_buffer_0.buffer_, merge_buffer_1.buffer_);

    merge_buffer_1.is_ready_to_fill_ = false;

    thread_pool_->add([&, size_in_bytes = merge_buffer_size_in_bytes]() {
      WriteFile(output_file_stream_, reinterpret_cast<const char*>(merge_buffer_1.buffer_.get()), size_in_bytes,
                output_file_path_);

      merge_buffer_1.is_ready_to_fill_ = true;
    });

    current_merge_buffer_index = 0;
  };

  NumberType number;
  NumberType top_value;
  MergeData<NumberType> min_data;

  while (!merge_queue.empty()) {
    if (current_merge_buffer_index == merge_numbers_count) {
      swapMergeBufferAndCreateTask(merge_buffer_0, merge_buffer_1, current_merge_buffer_index,
                                   merge_buffer_size_in_bytes);
    }

    min_data = merge_queue.top();
    merge_queue.pop();

    merge_buffer_0.buffer_[current_merge_buffer_index++] = min_data.value_;

    top_value = merge_queue.empty() ? std::numeric_limits<NumberType>::max() : merge_queue.top().value_;

    // Copies a number from the file buffer as long as it is the minimum.
    while (files_buffers[min_data.file_buffer_index_].get(number)) {
      if (number <= top_value) {
        if (current_merge_buffer_index == merge_numbers_count) {
          swapMergeBufferAndCreateTask(merge_buffer_0, merge_buffer_1, current_merge_buffer_index,
                                       merge_buffer_size_in_bytes);
        }

        merge_buffer_0.buffer_[current_merge_buffer_index++] = number;

        continue;
      }

      merge_queue.push(MergeData<NumberType>{min_data.file_buffer_index_, number});

      break;
    }
  }

  if (current_merge_buffer_index != 0) {
    WriteFile(output_file_stream_, reinterpret_cast<const char*>(merge_buffer_0.buffer_.get()),
              current_merge_buffer_index * sizeof(NumberType), output_file_path_);
  }

  thread_pool_->checkException();
}

template class ExternalSorter<number_t>;
}  // namespace es