
#
#	Makefile options
#

MAKEFLAGS += -j16

#
#	Compilation and linking options
#

CXX							= g++
CXXFLAGS					= -Wall -Werror -Wextra -std=c++17 -pedantic -g
LDFLAGS						= $(shell pkg-config --cflags --libs gtest) -lgtest_main
GCFLAGS						= -fprofile-arcs -ftest-coverage -fPIC
VGFLAGS						= --log-file="valgrind.txt" --track-origins=yes --trace-children=yes --leak-check=full --leak-resolution=med

#
#	Extensions
#

CPP							= .cc
TPP							= .tpp
HEADERS						= .h
OBJ							= .o

#
#	Libraries
#

MEMORY_LIB					= s21_memory.a

#
#	Connecting source file directories
#

VPATH						:= $(shell find . -type d)
vpath %$(CPP) $(VPATH)
vpath %$(HEADERS) $(VPATH)

#
#	Directories of source and object files
#

SRC_TESTS_DIR				= tests/
OBJ_DIR						= ../obj/
OBJ_TESTS_DIR				:= $(OBJ_DIR)$(SRC_TESTS_DIR)

#
#	Creating folders for object files
#

ALL_OBJ_DIRS				= $(OBJ_TESTS_DIR)
FOLDERS						:= $(foreach dir, $(ALL_OBJ_DIRS), $(shell mkdir -p $(dir)))

#
#	Parsing of source files from directories and subdirectories
#

SRC_TESTS					:= $(foreach dir, $(shell find $(SRC_TESTS_DIR) -type d), $(wildcard $(dir)/*$(CPP)))

#
#	Creating object files
#

OBJ_TESTS					:= $(patsubst %$(CPP), $(OBJ_TESTS_DIR)%$(OBJ), $(notdir $(SRC_TESTS)))

#
#	TARGETS
#

all: $(MEMORY_LIB) test

$(MEMORY_LIB):
	$(CXX) $(CXXFLAGS) -c Heap.cc
	ar rc $(MEMORY_LIB) Heap.o
	ranlib $(MEMORY_LIB)

cli: $(MEMORY_LIB)
	$(CXX) $(CXXFLAGS) ui.cc -o cli $(MEMORY_LIB)
	./cli

test: $(MEMORY_LIB) $(OBJ_TESTS)
	$(CXX) $(CXXFLAGS) $(OBJ_TESTS) -o test $(MEMORY_LIB) $(LDFLAGS)
	./test

coverage: $(MEMORY_LIB) $(OBJ_TESTS)
	$(CXX) $(CXXFLAGS) $(GCFLAGS) -o test $(OBJ_TESTS) --coverage Heap.cc $(LDFLAGS)
	./test
	lcov -t "test" -o report.info -c -d .
	genhtml -o report report.info

open_coverage_report:
	open report/index.html

valgrind: test
	valgrind $(VGFLAGS) ./test
	! grep -n "ERROR SUMMARY" valgrind.txt | grep -v "0 errors"

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(MEMORY_LIB)
	rm -rf cli
	rm -rf *$(OBJ)
	rm -rf test
	rm -rf valgrind.txt
	rm -rf report
	rm -rf *.info
	rm -rf *.gc*

$(OBJ_TESTS_DIR)%$(OBJ): %$(CPP)
	$(CXX) $(CXXFLAGS) -c $< -o $@

format_set:
	find . -iname "*$(CPP)" -o -iname "*$(HEADERS)" -o -iname "*$(TPP)" | xargs clang-format --style=google -i --verbose

format_check:
	find . -iname "*$(CPP)" -o -iname "*$(HEADERS)" -o -iname "*$(TPP)" | xargs clang-format --style=google -n --verbose

.PHONY: all test clean valgrind format_set format_check
