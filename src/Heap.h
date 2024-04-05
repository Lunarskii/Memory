#ifndef MEMORY_HEAP_H
#define MEMORY_HEAP_H

#include <chrono>
#include <cstdlib>
#include <memory>
#include <random>
#include <variant>
#include <vector>

namespace s21 {
class Heap {
 public:
  using heap_t = std::byte;
  enum class Type : unsigned char {
    Char,
    Int,
    Double,
  };
  struct Header {
    Header* next{};
    Header* prev{};
    bool state{};
    std::size_t size{};
    std::size_t alignment{};
    std::byte* addr{};
    Type type{};
  };

 public:
  Heap(const Heap&) = delete;
  Heap& operator=(const Heap&) = delete;

  static Heap& GetInstance(std::size_t size = 0);
  void* Malloc(std::size_t size);
  void* MallocOnlyFree(std::size_t size);
  void* Calloc(std::size_t num, std::size_t size);
  void* CallocOnlyFree(std::size_t num, std::size_t size);
  void Free(void* ptr);
  void* Realloc(void* ptr, std::size_t size);
  void* ReallocOnlyFree(void* ptr, std::size_t size);
  void Defragmentation();
  void Print();
  const Header* GetFirstHeader();
  void Write(void* ptr, Heap::Type type,
             const std::vector<std::variant<char, int, double>>& value);
  bool Empty();

 private:
  Heap() = default;

  constexpr static std::size_t header_size = sizeof(Header);
  constexpr static std::size_t machine_word = sizeof(std::size_t);

  void UpdateSize(size_t size);
  static std::size_t Align(std::size_t size) noexcept;
  static Header* FindPointer(void* ptr);
  void* SplitBlocks(Header* header, size_t new_current_block_size) noexcept;
  void* ExpOrMoveBlock(Header* header, size_t size);
  bool MergeBlocks(Header* header);
  template <class T>
  void PrintValue(std::byte* ptr, size_t size);
  template <class T>
  void WriteType(
      void* ptr,
      const std::vector<std::variant<char, int, double>>& value) const;

 private:
  std::unique_ptr<heap_t[]> heap_;
  heap_t* end_ = nullptr;
  std::vector<Header*> free_blocks_;
};

namespace Memory {
void s21_init(std::size_t size);
void* s21_malloc(std::size_t size);
void* s21_malloc_onlyfree(std::size_t size);
void* s21_calloc(std::size_t num, std::size_t size);
void* s21_calloc_onlyfree(std::size_t num, std::size_t size);
void s21_free(void* ptr);
void s21_free_onlyfree(void* ptr);
void* s21_realloc(void* ptr, std::size_t size);
void* s21_realloc_onlyfree(void* ptr, std::size_t size);
void s21_defragmentation();
std::pair<std::chrono::milliseconds, std::chrono::milliseconds> s21_research(
    std::size_t percent);
void RandomlyFreeBlocks(std::vector<int*>& blocks, std::size_t num_free_blocks);
const Heap::Header* s21_get_first_header();
void s21_print();
void s21_write_value(void* ptr, Heap::Type type,
                     const std::vector<std::variant<char, int, double>>& input);
}  // namespace Memory

}  // namespace s21

#endif  // MEMORY_HEAP_H
