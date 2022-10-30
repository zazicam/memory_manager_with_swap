CC=g++
FLAGS=-Wall -Werror -Wextra

all: memory_pool.o swap.o test.o check.o logger.o
	$(CC) $(FLAGS) memory_pool.o swap.o test.o logger.o -o memory_pool_test 
	$(CC) $(FLAGS) check.o -o check_result

memory_pool.o: memory_pool.hpp memory_pool.cpp
	$(CC) $(FLAGS) -c memory_pool.cpp

swap.o: swap.hpp swap.cpp
	$(CC) $(FLAGS) -c swap.cpp

test.o: test.cpp
	$(CC) $(FLAGS) -c test.cpp

check.o: check.cpp check.hpp
	$(CC) $(FLAGS) -c check.cpp

logger.o: logger.cpp logger.hpp
	$(CC) $(FLAGS) -c logger.cpp

build: clean all

clean:
	rm -rf *.o *.bin

run:
	./memory_pool_test
	./check_result
