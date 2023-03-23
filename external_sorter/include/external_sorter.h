/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#pragma once

#include "defines.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

namespace es {

class ThreadPool;

/**
 * Sorts numbers stored in binary file using limited amount of memory (available_memory) and writes results to output file.
 * It sorts chunks of data and then merge them
 * NOTE: intermediate files will be created in output directory
 *
 * @tparam NumberType type of numbers in a binary file
 */
template <typename NumberType>
class ExternalSorter {
 public:
  /**
   * Constructor
   * @param available_memory available memory for solving the tasks (in bytes)
   * @param input_file_path path to input file
   * @param output_directory_path path to output file
   * @param thread_pool thread pool
   */
  ExternalSorter(std::size_t available_memory, std::string input_file_path, std::string output_directory_path,
                 std::shared_ptr<ThreadPool> thread_pool);
  ~ExternalSorter();
  ExternalSorter(ExternalSorter&&) = default;
  ExternalSorter& operator=(ExternalSorter&&) = default;

 public:
  /**
   * Performs sorting and storing results to the output file
   */
  void sort();

 private:
  /**
   * Reads chunk of input file, then sorts this chunk and writes it to intermediate directory in single thread
   */
  void createSortedChunksImplSingleThreaded();

  /**
   * Reads chunk of input file, then sorts this chunk and writes it to intermediate directory in several threads using thread pool
   */
  void createSortedChunksImplMultiThreaded();

  /**
   * Merges sorted chunks from intermediate directory and writes results to output file
   */
  void mergeSortedChunksImpl();

 private:
  /**
   * Creates the intermediate directory
   */
  void createIntermediateDirectory() const;

 private:
  std::size_t available_memory_;                       ///< Amount of available memory
  std::string input_file_path_;                        ///< Input file path
  std::string output_directory_path_;                  ///< Path to output directory
  std::string output_file_path_;                       ///< Output file path
  std::filesystem::path intermediate_directory_path_;  ///< Path to intermediate directory

  std::ifstream input_file_stream_;   ///< input file stream
  std::ofstream output_file_stream_;  ///< output file stream

  std::shared_ptr<ThreadPool> thread_pool_;  ///< thread pool

  std::atomic_uint32_t intermediate_files_count_ = {};  ///< Amount of intermediate files
};

}  // namespace es