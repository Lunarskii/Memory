#include "Heap.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <variant>

namespace s21 {

Heap &Heap::GetInstance(std::size_t size) {
  static Heap instance;
  instance.UpdateSize(size);
  if (instance.Empty()) throw std::runtime_error("heap is empty");
  return instance;
}

void Heap::UpdateSize(size_t size) {
  if (!size) return;
  if (size < header_size + machine_word)
    throw std::runtime_error("Heap size is less than header size");
  if (heap_) {
    heap_.reset();
    free_blocks_.clear();
  }
  size += header_size;
  size += Align(size);
  heap_ = std::make_unique<heap_t[]>(size);
  auto header = new (heap_.get()) Header{nullptr,
                                         nullptr,
                                         false,
                                         size - header_size,
                                         0,
                                         heap_.get() + header_size,
                                         Heap::Type::Char};
  end_ = header->addr + header->size;
  free_blocks_.push_back(header);
}

void *Heap::Malloc(std::size_t size) {
  auto header = reinterpret_cast<Header *>(heap_.get());
  for (auto current = header; current; current = current->next) {
    if (!current->state && current->size >= size) {
      auto it = std::find_if(free_blocks_.begin(), free_blocks_.end(),
                             [&current](Header *x) { return x == current; });

      if (it != free_blocks_.end()) {
        free_blocks_.erase(it);
      }

      return SplitBlocks(current, size);
    }
  }

  return nullptr;
}

void *Heap::MallocOnlyFree(std::size_t size) {
  for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
    if ((*it)->size >= size) {
      auto header = *it;
      free_blocks_.erase(it);
      auto addr = SplitBlocks(header, size);
      return addr;
    }
  }

  return nullptr;
}

void *Heap::SplitBlocks(Header *header,
                        size_t new_current_block_size) noexcept {
  if (header->size != new_current_block_size) {
    auto end = header->addr + header->size + header->alignment;
    auto new_current_block_alignment =
        Align(new_current_block_size + header_size);

    header->size = new_current_block_size;
    header->alignment = new_current_block_alignment;
    auto new_header_byte = header->addr + header->size + header->alignment;
    auto space_left = end - new_header_byte;
    if (space_left < static_cast<long>(header_size + machine_word)) {
      header->alignment += space_left;
    } else {
      auto new_header =
          new (new_header_byte) Header{header->next,
                                       header,
                                       false,
                                       space_left - header_size,
                                       0,
                                       new_header_byte + header_size,
                                       Heap::Type::Char};
      if (header->next) header->next->prev = new_header;
      header->next = new_header;
      free_blocks_.push_back(new_header);
    }
  }
  header->state = true;

  return static_cast<void *>(header->addr);
}

std::size_t Heap::Align(std::size_t size) noexcept {
  if (!(size % machine_word)) return 0;
  return (size / machine_word + 1) * machine_word - size;
}

void *Heap::Calloc(std::size_t num, std::size_t size) {
  auto total_size = num * size;
  auto mem = Malloc(total_size);
  if (mem) {
    std::fill_n(reinterpret_cast<std::byte *>(mem), total_size, std::byte(0));
  }
  return mem;
}

void *Heap::CallocOnlyFree(std::size_t num, std::size_t size) {
  auto total_size = num * size;
  auto addr = MallocOnlyFree(total_size);

  if (addr) {
    std::fill_n(reinterpret_cast<std::byte *>(addr), total_size, std::byte(0));
  }

  return addr;
}

void Heap::Free(void *ptr) {
  auto header = FindPointer(ptr);
  if (header) {
    header->state = false;
    header->size += header->alignment;
    header->alignment = 0;
    free_blocks_.push_back(header);
  }
}

void *Heap::Realloc(void *ptr, std::size_t size) {
  auto header = FindPointer(ptr);
  return (header == nullptr) ? Malloc(size) : ExpOrMoveBlock(header, size);
}

void *Heap::ReallocOnlyFree(void *ptr, std::size_t size) {
  auto header = FindPointer(ptr);
  return (header == nullptr) ? MallocOnlyFree(size)
                             : ExpOrMoveBlock(header, size);
}

Heap::Header *Heap::FindPointer(void *ptr) {
  if (!ptr) return nullptr;
  auto current =
      reinterpret_cast<Header *>(static_cast<std::byte *>(ptr) - header_size);
  if (!current->state) throw std::runtime_error("wrong pointer");
  return current;
}

void *Heap::ExpOrMoveBlock(Heap::Header *header, size_t size) {
  while (size > header->size && MergeBlocks(header))
    ;

  if (size <= header->size) {
    return SplitBlocks(header, size);
  } else {
    auto new_ptr = MallocOnlyFree(size);
    if (new_ptr) {
      std::copy_n(header->addr, header->size,
                  static_cast<std::byte *>(new_ptr));
      header->state = false;
      free_blocks_.push_back(header);
    }
    return new_ptr;
  }
}

bool Heap::MergeBlocks(Heap::Header *header) {
  if (header->next && !header->next->state) {
    auto it = std::find_if(free_blocks_.begin(), free_blocks_.end(),
                           [&header](Header *x) { return x == header->next; });

    if (it != free_blocks_.end()) {
      free_blocks_.erase(it);
    }

    header->size += header->alignment + header->next->size +
                    header->next->alignment + header_size;
    header->alignment = 0;
    header->next = header->next->next;
    if (header->next) header->next->prev = header;
    return true;
  }

  return false;
}

void Heap::Defragmentation() {
  Header *previous = nullptr;
  size_t memory_shift = 0;

  for (auto current = reinterpret_cast<Header *>(heap_.get()); current;
       current = current->next) {
    if (current->state) {
      std::size_t extra_memory_of_current_block = 0;
      if (current->alignment > machine_word) {
        extra_memory_of_current_block =
            current->alignment - Align(current->size + header_size);
        current->alignment -= extra_memory_of_current_block;
      }
      auto byte_ptr = current->addr - header_size;
      std::copy_n(byte_ptr, header_size + current->size + current->alignment,
                  byte_ptr - memory_shift);
      current = reinterpret_cast<Header *>(byte_ptr - memory_shift);
      current->addr = byte_ptr - memory_shift + header_size;
      current->prev = previous;
      if (previous) previous->next = current;
      previous = current;
      memory_shift += extra_memory_of_current_block;
    } else {
      memory_shift += header_size + current->size;
    }
  }
  free_blocks_.clear();
  if (previous && memory_shift) {
    auto start_of_free_space =
        previous->addr + previous->size + previous->alignment;
    size_t size_of_free_space = end_ - start_of_free_space;
    if (size_of_free_space < header_size + machine_word) {
      previous->alignment += size_of_free_space;
    } else {
      auto new_header =
          new (start_of_free_space) Header{nullptr,
                                           previous,
                                           false,
                                           size_of_free_space - header_size,
                                           0,
                                           start_of_free_space + header_size,
                                           Heap::Type::Char};
      previous->next = new_header;
      free_blocks_.push_back(new_header);
    }
  }
}

void Heap::Print() {
  auto header = reinterpret_cast<Header *>(heap_.get());
  for (auto current = header; current; current = current->next) {
    std::cout << current->addr << "\n";
    std::cout << "\tContent: ";
    switch (current->type) {
      case Type::Char:
        PrintValue<char>(current->addr, current->size);
        break;
      case Type::Int:
        PrintValue<int>(current->addr, current->size);
        break;
      case Type::Double:
        PrintValue<double>(current->addr, current->size);
        break;
    }
    std::cout << "\n"
              << "\tSize: " << current->size << "\n\tState: " << current->state
              << "\n";
  }
}

template <class T>
void Heap::PrintValue(std::byte *ptr, size_t size) {
  size_t num_of_elms = size / sizeof(T);
  bool array = num_of_elms > 1, char_type = typeid(T) == typeid(char);
  if (array) std::cout << "[";
  for (size_t i = 0, ie = num_of_elms; i < ie; ++i) {
    if (char_type) std::cout << '\'';
    std::cout << reinterpret_cast<T *>(ptr)[i];
    if (char_type) std::cout << '\'';
    if (i < ie - 1) std::cout << ", ";
  }
  if (array) std::cout << "]";
}

void Heap::Write(void *ptr, Heap::Type type,
                 const std::vector<std::variant<char, int, double>> &value) {
  auto header =
      reinterpret_cast<Header *>(static_cast<std::byte *>(ptr) - header_size);
  switch (type) {
    case Type::Char:
      WriteType<char>(ptr, value);
      break;
    case Type::Int:
      WriteType<int>(ptr, value);
      break;
    case Type::Double:
      WriteType<double>(ptr, value);
      break;
  }
  header->type = type;
}

const Heap::Header *Heap::GetFirstHeader() {
  return reinterpret_cast<Header *>(heap_.get());
}

bool Heap::Empty() { return heap_ == nullptr; }

template <class T>
void Heap::WriteType(
    void *ptr,
    const std::vector<std::variant<char, int, double>> &value) const {
  auto type_ptr = reinterpret_cast<T *>(ptr);
  for (auto const &elm : value) {
    auto dbl_num = std::get<double>(elm);
    *type_ptr = static_cast<T>(dbl_num);
    ++type_ptr;
  }
}

void *Memory::s21_malloc(std::size_t size) {
  return Heap::GetInstance().Malloc(size);
}

void *Memory::s21_malloc_onlyfree(std::size_t size) {
  return Heap::GetInstance().MallocOnlyFree(size);
}

void *Memory::s21_calloc(std::size_t num, std::size_t size) {
  return Heap::GetInstance().Calloc(num, size);
}

void *Memory::s21_calloc_onlyfree(std::size_t num, std::size_t size) {
  return Heap::GetInstance().CallocOnlyFree(num, size);
}

void Memory::s21_free(void *ptr) { Heap::GetInstance().Free(ptr); }

void Memory::s21_free_onlyfree(void *ptr) { Heap::GetInstance().Free(ptr); }

void *Memory::s21_realloc(void *ptr, std::size_t size) {
  return Heap::GetInstance().Realloc(ptr, size);
}

void *Memory::s21_realloc_onlyfree(void *ptr, std::size_t size) {
  return Heap::GetInstance().ReallocOnlyFree(ptr, size);
}

void Memory::s21_defragmentation() { Heap::GetInstance().Defragmentation(); }

std::pair<std::chrono::milliseconds, std::chrono::milliseconds>
Memory::s21_research(std::size_t percent) {
  if (percent < 1 || percent > 100) {
    throw std::invalid_argument("The research doesn't make sense");
  }

  std::pair<std::chrono::milliseconds, std::chrono::milliseconds> time;
  std::vector<int *> vector;
  int *x;

  s21_init(1'000'000);
  do {
    x = reinterpret_cast<int *>(s21_malloc(10));
    if (x != nullptr) {
      vector.push_back(x);
    }
  } while (x != nullptr);
  auto num_free_blocks = vector.size() / 100 * percent;
  RandomlyFreeBlocks(vector, num_free_blocks);

  auto start_time = std::chrono::high_resolution_clock::now();
  do {
    x = reinterpret_cast<int *>(s21_malloc(10));
  } while (x != nullptr);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto time_of_first_block =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time);
  vector.clear();

  s21_init(1'000'000);
  do {
    x = reinterpret_cast<int *>(s21_malloc_onlyfree(10));
    if (x != nullptr) {
      vector.push_back(x);
    }
  } while (x != nullptr);
  RandomlyFreeBlocks(vector, num_free_blocks);

  start_time = std::chrono::high_resolution_clock::now();
  do {
    x = reinterpret_cast<int *>(s21_malloc_onlyfree(10));
  } while (x != nullptr);
  end_time = std::chrono::high_resolution_clock::now();
  auto time_of_second_block =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                            start_time);

  return std::make_pair(time_of_first_block, time_of_second_block);
}

void Memory::RandomlyFreeBlocks(std::vector<int *> &blocks,
                                std::size_t num_free_blocks) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 1);
  std::size_t free_blocks_counter = 0;

  while (true) {
    for (auto &block : blocks) {
      if (block != nullptr && dist(gen)) {
        s21_free_onlyfree(block);
        block = nullptr;
        ++free_blocks_counter;

        if (free_blocks_counter == num_free_blocks) {
          return;
        }
      }
    }
  }
}

void Memory::s21_init(std::size_t size) { Heap::GetInstance(size); }

void Memory::s21_write_value(
    void *ptr, s21::Heap::Type type,
    const std::vector<std::variant<char, int, double>> &input) {
  Heap::GetInstance().Write(ptr, type, input);
}

void Memory::s21_print() { Heap::GetInstance().Print(); }

const Heap::Header *Memory::s21_get_first_header() {
  return Heap::GetInstance().GetFirstHeader();
}
}  // namespace s21