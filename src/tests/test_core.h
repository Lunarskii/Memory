#ifndef MEMORY_TESTS_TEST_CORE_H_
#define MEMORY_TESTS_TEST_CORE_H_

#include <gtest/gtest.h>

#include "../Heap.h"

namespace Test {

using namespace s21::Memory;
using size_type = std::size_t;
constexpr static size_type header_size = sizeof(s21::Heap::Header);

class MemoryTests : public ::testing::Test {
 protected:
  constexpr static size_type int_size = sizeof(int);
  constexpr static size_type num_elements = 128;
};

}  // namespace Test

#endif  // MEMORY_TESTS_TEST_CORE_H_
