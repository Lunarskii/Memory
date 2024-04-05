//
// Created by ruslan on 15.11.23.
//

#include <iostream>
#include <limits>
#include <variant>

#include "Heap.h"

using namespace s21::Memory;

bool ProcessMenu();

void ProcessFunction();

void WriteValue();

int main() {
  while (ProcessMenu()) {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.clear();
  }
  return 0;
}

bool ProcessMenu() {
  std::cout << "1. Size of heap to initialize\n"
               "2. Call one of functions\n"
               "3. Writing a value\n"
               "4. Output of the current heap\n"
               "5. Research\n"
               "6. Defragmentation\n"
               "7. Exit\n";
  int ch;
  std::cin >> ch;
  try {
    switch (ch) {
      case 1: {
        std::cout << "Enter size:\n";
        size_t size;
        std::cin >> size;
        s21_init(size);
        std::cout << "Inited\n";
      } break;
      case 2:
        ProcessFunction();
        break;
      case 3:
        WriteValue();
        break;
      case 4:
        s21_print();
        break;
      case 5: {
        std::cout << "Enter percent of free blocks\n";
        std::size_t percent;
        while (!(std::cin >> percent) || percent < 5 || percent > 95) {
          std::cout << "try again\n";
        }
        auto [without_only, with_only] = s21_research(percent);
        std::cout << "Time with common funcs : \t" << without_only.count();
        std::cout << "\nTime with vector of free blocks : \t"
                  << with_only.count() << "\n";
      } break;
      case 6:
        s21_defragmentation();
        std::cout << "Defragmented\n";
        break;
      case 7:
        return false;
      default:
        std::cout << "try again...\n";
    }
  } catch (std::exception &e) {
    std::cout << "Error: " << e.what() << "\n";
  }
  return true;
}

void ProcessFunction() {
  std::cout << "Choose the function to call\n"
               "1. s21_malloc\n"
               "2. s21_calloc\n"
               "3. s21_realloc\n"
               "4. s21_free\n";
  int ch;
  while ((std::cin >> ch) && (ch < 1 || ch > 4))
    ;
  switch (ch) {
    case 1: {
      std::cout << "Enter size\n";
      size_t size;
      std::cin >> size;
      std::cout << s21_malloc(size) << "\n";
    } break;
    case 2: {
      std::cout << "Enter size\n";
      size_t size, num;
      std::cin >> size;
      std::cout << "Enter num\n";
      std::cin >> num;
      std::cout << s21_calloc(num, size) << "\n";
    } break;
    case 3: {
      std::cout << "Enter size\n";
      size_t size;
      void *ptr;
      std::cin >> size;
      std::cout << "Enter address\n";
      std::cin >> ptr;
      std::cout << s21_realloc(ptr, size) << "\n";
    } break;
    default: {
      std::cout << "Enter address\n";
      void *ptr;
      std::cin >> ptr;
      s21_free(ptr);
    }
  }
}

void WriteValue() {
  std::cout << "Enter address\n";
  uintptr_t addr;
  std::cin >> std::hex >> addr;
  void *ptr = reinterpret_cast<void *>(addr);
  std::cout << "Enter type\n"
               "1. char\n"
               "2. int\n"
               "3. double\n";
  s21::Heap::Type type;
  int ch;
  while (!(std::cin >> ch) || ch < 1 || ch > 3)
    ;
  type = static_cast<s21::Heap::Type>(ch - 1);
  std::cout << "Enter number of elements\n";
  size_t size;
  std::cin >> size;
  std::vector<std::variant<char, int, double>> input;
  for (size_t i = 0; i < size; ++i) {
    double num;
    std::cout << "Enter " << i + 1 << " element:\n";
    std::cin >> num;
    input.emplace_back(num);
  }
  s21_write_value(ptr, type, input);
}