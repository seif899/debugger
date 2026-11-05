CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g -O0
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)

mydbg: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp $(wildcard src/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Sample target (non-PIE, so addresses are fixed)
tests/target: tests/target.c
	gcc -g -O0 -no-pie -o $@ $<

clean:
	rm -rf build mydbg tests/target

.PHONY: clean