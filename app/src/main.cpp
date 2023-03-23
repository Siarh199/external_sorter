/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include <external_sorter/include/defines.h>
#include <external_sorter/include/external_sorter.h>
#include <external_sorter/include/thread_pool.h>

#include <exception>
#include <iostream>
#include <memory>

namespace {

enum : int { kOk = 0, kError = -1 };

const std::size_t kMemorySize = 128 * 1024 * 1024;  // 128mb
const std::string kDefaultInputPath = "input";
const std::string kDefaultOutputDirectory = "./";

}  // namespace

int main() {
  try {
    es::ExternalSorter<es::number_t> sorter{kMemorySize, kDefaultInputPath,
                                            kDefaultOutputDirectory,
                                            std::make_shared<es::ThreadPool>()};

    sorter.sort();
  } catch (const std::exception& ex) {
    std::cout << "Exception occurred: " << ex.what() << ".\n";

    return kError;
  }

  return kOk;
}