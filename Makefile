CC=clang++
CXXFLAGS=-std=c++20 -ggdb

all: main

main: main.cpp stack.hpp bucket_storage.hpp
	$(CC) $(CXXFLAGS) -lgtest -Wall -Wextra -Wpedantic -Wno-self-assign-overloaded main.cpp -o main

clean:
	rm -rf main
