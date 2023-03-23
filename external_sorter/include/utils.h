/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#pragma once

#include <cerrno>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace es {

/**
 * Rounds size to count of numbers
 * @tparam NumberType type of number
 * @param size size
 * @return rounded size
 */
template <typename NumberType>
std::size_t RoundSize(std::size_t size) noexcept {
  return size / sizeof(NumberType) * sizeof(NumberType);
}

/**
 * Results of reading a file
 */
struct FileReadResult {
  bool ok_ = {};
  std::size_t bytes_count_ = {};
};

/**
 * Reads binary file to a buffer
 * @param stream input file stream
 * @param buffer buffer
 * @param buffer_size size of the buffer
 * @return reading results
 */
FileReadResult ReadFileStream(std::ifstream& stream, char* buffer, std::size_t buffer_size);
/**
 * Opens input binary file
 * @param file_path path to file
 * @return input stream
 */
std::ifstream OpenInputBinaryFileStream(std::string_view file_path);

/**
 * Opens output binary file
 * @param file_path path to file
 * @return output stream
 */
std::ofstream OpenOutputBinaryFileStream(std::string_view file_path);

/**
 * Exception type alias
 */
using exception_t = std::runtime_error;

/**
 * Helper type for using in static_assert
 * TODO: consider removing after switching to C++20
 * @tparam T
 */
template <class T>
struct dependent_false : std::false_type {};

/**
 * Creates an exception
 * @param message exception message
 * @return exception
 */
exception_t MakeException(std::string message);

/**
 * Creates an exception
 * TODO consider using fold expressions
 * @tparam T first argument type
 * @tparam Args other arguments types
 * @param message exception message
 * @param arg first argument
 * @param args other arguments
 * @return exception
 */
template <typename T, typename... Args>
exception_t MakeException(std::string message, T&& arg, Args&&... args) {
  using decay_type = std::decay_t<T>;

  if constexpr (std::is_same_v<decay_type, std::string> || std::is_same_v<decay_type, std::string_view>) {
    message.append(arg);
  } else if constexpr (std::is_same_v<decay_type, std::error_code>) {
    message.append(std::system_error{arg}.what());
  } else if constexpr (std::is_same_v<decay_type, int>) {
    message.append(std::strerror(arg));
  } else {
    static_assert(dependent_false<T>::value, "Unsupported parameter type");
  }

  return MakeException(std::move(message), std::forward<Args>(args)...);
}

}  // namespace es