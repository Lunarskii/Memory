#include "test_core.h"

namespace Test {

void func(const std::vector<int *> &vars, const s21::Heap::Header *header,
          size_type num_elements, size_type num_free_elements,
          size_type num_elements_in_one, size_type type_size,
          [[maybe_unused]] size_type alignment, bool initialize) {
  if (initialize) {
    for (size_type i = 0; i < num_elements - num_free_elements; ++i) {
      if (vars[i] == nullptr) {
        --num_elements;
      } else {
        *vars[i] = static_cast<int>(i + 1);
      }
    }
  }

  size_type i = 0;
  for (auto current = header; current; current = current->next) {
    if (i < num_elements - num_free_elements) {
      std::byte *addr = current->addr;
      size_type one_element_size = current->size / num_elements_in_one;
      auto *data = new std::byte[one_element_size];

      for (size_type j = 0; j < num_elements_in_one; ++j) {
        std::copy(addr, addr + one_element_size, data);
        if (j != num_elements_in_one - 1) {
          addr += one_element_size;
        }

        if (initialize) {
          EXPECT_EQ(*reinterpret_cast<int *>(data), *vars[i++]);
        } else {
          EXPECT_EQ(*reinterpret_cast<int *>(data), 0);
        }
      }
      delete[] data;

      EXPECT_EQ(current->state, true);
      EXPECT_EQ(current->size, type_size * num_elements_in_one);

      if (current->next) {
        EXPECT_EQ(current->next->addr - header_size,
                  current->addr + current->size + current->alignment);
      }

      //            if (num_free_elements == 0 && i == num_elements - 1)
      //            {
      //                EXPECT_EQ(current->alignment, )
      //            }
      //            if (i != num_elements - num_free_elements - 1)
      //            {
      //                EXPECT_EQ(current->alignment, alignment);
      //            }

      //            EXPECT_EQ(current->type, s21::Heap::Type::Int);
    } else {
      EXPECT_EQ(current->state, false);
    }
  }
}

TEST_F(MemoryTests, MallocFullyOccupiedHeap) {
  s21_init((int_size + header_size) * num_elements);
  std::vector<int *> vars{num_elements, nullptr};

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = reinterpret_cast<int *>(s21_malloc(int_size));
  }

  func(vars, s21_get_first_header(), num_elements, 0, 1, int_size, int_size,
       true);
}

TEST_F(MemoryTests, MallocNotFullyOccupiedHeap) {
  s21_init((int_size + header_size) * num_elements);
  std::vector<int *> vars{num_elements - 1, nullptr};

  for (size_type i = 0; i < num_elements - 1; ++i) {
    vars[i] = reinterpret_cast<int *>(s21_malloc(int_size));
  }

  func(vars, s21_get_first_header(), num_elements, 1, 1, int_size, int_size,
       true);
}

TEST_F(MemoryTests, CallocWithoutInitialization) {
  s21_init(header_size + int_size * num_elements);
  int *arr = reinterpret_cast<int *>(s21_calloc(num_elements, int_size));
  std::vector<int *> vars(num_elements, nullptr);

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 0, num_elements, int_size,
       int_size, true);
}

TEST_F(MemoryTests, CallocWithInitialization) {
  s21_init(header_size + int_size * num_elements);
  int *arr = reinterpret_cast<int *>(s21_calloc(num_elements, int_size));
  std::vector<int *> vars(num_elements, nullptr);

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 0, num_elements, int_size,
       int_size, false);
}

TEST_F(MemoryTests, CallocNotFullyOccupiedHeap) {
  s21_init(header_size + int_size * num_elements);
  int *arr = reinterpret_cast<int *>(s21_calloc(num_elements - 1, int_size));
  std::vector<int *> vars(num_elements - 1, nullptr);

  for (size_type i = 0; i < num_elements - 1; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 1, num_elements - 1,
       int_size, int_size, true);
}

TEST_F(MemoryTests, ReallocEmptyPointer) {
  s21_init(64);
  int *x = nullptr;
  EXPECT_TRUE(s21_realloc(x, int_size) != nullptr);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocImpossibleToIncrease) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc(int_size));
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  s21_realloc(x, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocIncrease) {
  s21_init(128);
  int *x = reinterpret_cast<int *>(s21_malloc(int_size));
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  s21_realloc(x, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->size, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocReduce) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc(int_size * 2));
  EXPECT_EQ(s21_get_first_header()->size, int_size * 2);
  s21_realloc(x, int_size);
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocNoNearestFreeBlock) {
  s21_init(256);
  int *x = reinterpret_cast<int *>(s21_malloc(int_size));
  s21_malloc(int_size);
  s21_realloc(x, int_size * 3);
  auto header = s21_get_first_header();
  EXPECT_FALSE(header->state);
  header = header->next;
  EXPECT_TRUE(header->state);
  header = header->next;
  EXPECT_TRUE(header->state);
  EXPECT_EQ(header->size, int_size * 3);
}

TEST_F(MemoryTests, FreeNullptr) {
  int *x = nullptr;
  s21_free(x);
}

TEST_F(MemoryTests, FreeInvalidPointer) {
  int x = 10;
  int *y = &x;
  EXPECT_ANY_THROW(s21_free(y));
}

TEST_F(MemoryTests, Free) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc(int_size));
  s21_free(x);
  auto header = s21_get_first_header();
  EXPECT_TRUE(header->next == nullptr);
  EXPECT_FALSE(header->state);
}

TEST_F(MemoryTests, MallocOnlyFreeFullyOccupiedHeap) {
  s21_init((int_size + header_size) * num_elements);
  std::vector<int *> vars{num_elements, nullptr};

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  }

  func(vars, s21_get_first_header(), num_elements, 0, 1, int_size, int_size,
       true);
}

TEST_F(MemoryTests, MallocOnlyFreeNotFullyOccupiedHeap) {
  s21_init((int_size + header_size) * num_elements);
  std::vector<int *> vars{num_elements - 1, nullptr};

  for (size_type i = 0; i < num_elements - 1; ++i) {
    vars[i] = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  }

  func(vars, s21_get_first_header(), num_elements, 1, 1, int_size, int_size,
       true);
}

TEST_F(MemoryTests, CallocOnlyFreeWithoutInitialization) {
  s21_init(header_size + int_size * num_elements);
  int *arr =
      reinterpret_cast<int *>(s21_calloc_onlyfree(num_elements, int_size));
  std::vector<int *> vars(num_elements, nullptr);

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 0, num_elements, int_size,
       int_size, true);
}

TEST_F(MemoryTests, CallocOnlyFreeWithInitialization) {
  s21_init(header_size + int_size * num_elements);
  int *arr =
      reinterpret_cast<int *>(s21_calloc_onlyfree(num_elements, int_size));
  std::vector<int *> vars(num_elements, nullptr);

  for (size_type i = 0; i < num_elements; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 0, num_elements, int_size,
       int_size, false);
}

TEST_F(MemoryTests, CallocOnlyFreeNotFullyOccupiedHeap) {
  s21_init(header_size + int_size * num_elements);
  int *arr =
      reinterpret_cast<int *>(s21_calloc_onlyfree(num_elements - 1, int_size));
  std::vector<int *> vars(num_elements - 1, nullptr);

  for (size_type i = 0; i < num_elements - 1; ++i) {
    vars[i] = &arr[i];
  }

  func(vars, s21_get_first_header(), num_elements, 1, num_elements - 1,
       int_size, int_size, true);
}

TEST_F(MemoryTests, ReallocOnlyFreeEmptyPointer) {
  s21_init(64);
  int *x = nullptr;
  EXPECT_TRUE(s21_realloc_onlyfree(x, int_size) != nullptr);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocOnlyFreeImpossibleToIncrease) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  s21_realloc_onlyfree(x, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocOnlyFreeIncrease) {
  s21_init(128);
  int *x = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  s21_realloc_onlyfree(x, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->size, int_size * 3);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocOnlyFreeReduce) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size * 2));
  EXPECT_EQ(s21_get_first_header()->size, int_size * 2);
  s21_realloc_onlyfree(x, int_size);
  EXPECT_EQ(s21_get_first_header()->size, int_size);
  EXPECT_EQ(s21_get_first_header()->state, true);
}

TEST_F(MemoryTests, ReallocOnlyFreeNoNearestFreeBlock) {
  s21_init(256);
  int *x = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  s21_malloc_onlyfree(int_size);
  s21_realloc_onlyfree(x, int_size * 3);
  auto header = s21_get_first_header();
  EXPECT_FALSE(header->state);
  header = header->next;
  EXPECT_TRUE(header->state);
  header = header->next;
  EXPECT_TRUE(header->state);
  EXPECT_EQ(header->size, int_size * 3);
}

TEST_F(MemoryTests, FreeOnlyFreeNullptr) {
  int *x = nullptr;
  s21_free_onlyfree(x);
}

TEST_F(MemoryTests, FreeOnlyFreeInvalidPointer) {
  int x = 10;
  int *y = &x;
  EXPECT_ANY_THROW(s21_free_onlyfree(y));
}

TEST_F(MemoryTests, FreeOnlyFree) {
  s21_init(64);
  int *x = reinterpret_cast<int *>(s21_malloc_onlyfree(int_size));
  s21_free_onlyfree(x);
  auto header = s21_get_first_header();
  EXPECT_TRUE(header->next == nullptr);
  EXPECT_FALSE(header->state);
}

TEST_F(MemoryTests, Defragmentation_00) {
  std::vector<int *> addresses;
  s21_init(num_elements * (2 * int_size + header_size) - header_size);
  for (size_t i = 0; i < num_elements; ++i) {
    addresses.push_back(
        reinterpret_cast<int *>(s21_malloc_onlyfree(2 * int_size)));
  }
  s21_defragmentation();
  func(addresses, s21_get_first_header(), num_elements, 0, 2, int_size, 0,
       false);
}
TEST_F(MemoryTests, Defragmentation_01) {
  std::vector<int *> addresses;
  s21_init(10 * (2 * int_size + header_size) - header_size);
  for (int i = 0; i < 10; ++i) {
    addresses.push_back(
        reinterpret_cast<int *>(s21_malloc_onlyfree(2 * int_size)));
  }
  auto free_elems = 5;
  RandomlyFreeBlocks(addresses, free_elems);
  s21_defragmentation();
  auto current = s21_get_first_header();
  for (int i = 5; i; --i, current = current->next) {
    EXPECT_TRUE(current->state);
    EXPECT_EQ(current->size, 2 * int_size);
    EXPECT_EQ(current->alignment, 0);
  }
  EXPECT_EQ(current->size, 264);
  EXPECT_FALSE(current->state);
}

TEST_F(MemoryTests, Defragmentation_02) {
  std::vector<int *> addresses;
  s21_init(4 * (128 + header_size) - header_size);
  for (int i = 0; i < 4; ++i) {
    addresses.push_back(reinterpret_cast<int *>(s21_malloc_onlyfree(128)));
  }
  s21_free_onlyfree(addresses[0]);
  s21_malloc_onlyfree(80);
  s21_free_onlyfree(addresses[2]);
  s21_malloc_onlyfree(80);
  s21_defragmentation();
  auto header = s21_get_first_header();
  for (int i = 4; i && header; --i) {
    header = header->next;
  }
  EXPECT_FALSE(header->state);
  EXPECT_EQ(header->size, 40);
}

TEST_F(MemoryTests, Defragmentation_03) {
  std::vector<int *> addresses;
  s21_init(2 * (128 + header_size) - header_size);
  for (int i = 0; i < 2; ++i) {
    addresses.push_back(reinterpret_cast<int *>(s21_malloc_onlyfree(128)));
  }
  s21_free_onlyfree(addresses[0]);
  s21_malloc_onlyfree(80);
  s21_defragmentation();
  auto header = s21_get_first_header();
  header = header->next;
  EXPECT_TRUE(header->state);
  EXPECT_EQ(header->alignment, 48);
}
}  // namespace Test
