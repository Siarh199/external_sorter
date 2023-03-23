/*
 * external_sorter: 2021 Sergey Gorelyshev
 */

#include <external_sorter/include/external_sorter.h>
#include <external_sorter/include/thread_pool.h>
#include <external_sorter/include/utils.h>

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <random>

namespace {

/**
 * Test fixture
 */
class ExternalSorterTests : public ::testing::Test {
 protected:
  static inline const std::size_t kMemorySize = 10 * 1024 * 1024;  // 10mb
  static inline const std::string kDefaultInputPath = "test/input";
  static inline const std::string kDefaultOutputDirectory = "test/output/";

  static inline const es::number_t kMin = 0;
  static inline const es::number_t kMax = 50000;

  /**
   * Test setup
   */
  void SetUp() override {
    std::error_code ec{};
    std::filesystem::create_directory("test", ec);
    std::filesystem::create_directory("test/output", ec);
  }

  /**
   * Test tear down
   */
  void TearDown() override {
    sorter_.reset();

    std::error_code ec{};
    std::filesystem::remove_all("test", ec);
  }

  void generateInputFile(std::size_t size) {
    auto stream{es::OpenOutputBinaryFileStream(kDefaultInputPath)};

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::uniform_int_distribution<es::number_t> distrib(kMin, kMax);

    for (std::size_t i = 0; i < size / sizeof(es::number_t); ++i) {
      es::number_t val = distrib(gen);
      stream.write(reinterpret_cast<char*>(&val), sizeof(val));
    }
  }

  bool checkOutputFile() {
    auto stream{es::OpenInputBinaryFileStream(kDefaultOutputDirectory + "output")};

    es::number_t prev = kMin;

    while (stream.good()) {
      es::number_t curr{};
      stream.read(reinterpret_cast<char*>(&curr), sizeof(curr));

      if (stream.good()) {
        if (curr < prev) {
          return false;
        }

        prev = curr;
      }
    }

    return true;
  }

  std::unique_ptr<es::ExternalSorter<es::number_t>> sorter_;
};

/**
 * Asserts that it is possible to sort a 'big' file
 */
TEST_F(ExternalSorterTests, sanity) {
  generateInputFile(kMemorySize * 10);

  sorter_ = std::make_unique<es::ExternalSorter<es::number_t>>(kMemorySize, kDefaultInputPath, kDefaultOutputDirectory,
                                                               std::make_shared<es::ThreadPool>());

  sorter_->sort();

  EXPECT_TRUE(checkOutputFile());
}

/**
 * Asserts that it is possible to sort a not 'big' file
 */
TEST_F(ExternalSorterTests, equalFile) {
  generateInputFile(kMemorySize);

  sorter_ = std::make_unique<es::ExternalSorter<es::number_t>>(kMemorySize, kDefaultInputPath, kDefaultOutputDirectory,
                                                               std::make_shared<es::ThreadPool>());

  sorter_->sort();

  EXPECT_TRUE(checkOutputFile());
}

/**
 * Asserts that it is possible to sort a 'small' file
 */
TEST_F(ExternalSorterTests, smallFile) {
  generateInputFile(kMemorySize / 3);

  sorter_ = std::make_unique<es::ExternalSorter<es::number_t>>(kMemorySize, kDefaultInputPath, kDefaultOutputDirectory,
                                                               std::make_shared<es::ThreadPool>());

  sorter_->sort();

  EXPECT_TRUE(checkOutputFile());
}

/**
 * Asserts that it is possible to sort an 'empty' file
 */
TEST_F(ExternalSorterTests, emptyFile) {
  generateInputFile(0);

  sorter_ = std::make_unique<es::ExternalSorter<es::number_t>>(kMemorySize, kDefaultInputPath, kDefaultOutputDirectory,
                                                               std::make_shared<es::ThreadPool>());

  sorter_->sort();

  EXPECT_TRUE(checkOutputFile());
}

}  // namespace