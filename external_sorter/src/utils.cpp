/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include "utils.h"

#include <cstring>
#include <exception>

namespace es {

namespace {

/**
 * Helper function for opening a file stream
 * @tparam stream_type
 * @param file_path
 * @return
 */
template <typename stream_type>
stream_type OpenBinaryFileStream(std::string_view file_path) {
  stream_type stream(file_path.data(), stream_type::binary);

  if (!stream.is_open()) {
    throw MakeException("Failed to open the file ", file_path, std::string_view{": "}, errno);
  }

  return stream;
}
}  // namespace

FileReadResult ReadFileStream(std::ifstream& stream, char* buffer, std::size_t buffer_size) {
  stream.read(buffer, buffer_size);

  auto stream_state = stream.rdstate();

  if (stream_state == std::ios_base::goodbit) {
    return {true, buffer_size};
  }

  if (stream_state == (std::ios_base::failbit | std::ios_base::eofbit)) {
    return {true, static_cast<std::size_t>(stream.gcount())};
  }

  return {false, 0};
}

std::ifstream OpenInputBinaryFileStream(std::string_view file_path) {
  return OpenBinaryFileStream<std::ifstream>(file_path);
}
std::ofstream OpenOutputBinaryFileStream(std::string_view file_path) {
  return OpenBinaryFileStream<std::ofstream>(file_path);
}

exception_t MakeException(std::string message) {
  return exception_t{message};
}

}  // namespace es