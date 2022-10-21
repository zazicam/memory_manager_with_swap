CC=g++
FLAGS=-Wall -Werror -Wextra

all: memory_pool.o swap.o test.o check.o
	$(CC) $(FLAGS) memory_pool.o swap.o test.o check.o -o memory_pool_test

memory_pool.o: memory_pool.hpp memory_pool.cpp
	$(CC) $(FLAGS) -c memory_pool.cpp

swap.o: swap.hpp swap.cpp
	$(CC) $(FLAGS) -c swap.cpp

test.o: test.cpp
	$(CC) $(FLAGS) -c test.cpp

check.o: check.cpp check.hpp
	$(CC) $(FLAGS) -c check.cpp

build: clean all

clean:
	rm -rf *.o *.bin

run:
	./memory_pool_test
