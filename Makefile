CC=clang++
CXXFLAGS=-std=c++20 -ggdb -Wall -Wextra -Wpedantic -Wno-self-assign-overloaded 

all: main

main: main.cpp stack.hpp bucket_storage.hpp
	$(CC) $(CXXFLAGS) -lgtest main.cpp -o main

.PHONY: memory benchmark

# make -B memory to recompile
memory: CXXFLAGS += -fsanitize=address -O0
memory: main

benchmark: CXXFLAGS += -O2
benchmark: main

clean:
	rm -rf main
