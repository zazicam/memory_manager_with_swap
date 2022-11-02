CC=g++
FLAGS=-Wall -Werror -Wextra

all: memory_pool_test memory_manager_test check_result

memory_manager_test: memory_manager.o memory_pool.o swap.o logger.o test.o 
	$(CC) $(FLAGS) memory_manager.o memory_pool.o swap.o test.o logger.o -o memory_manager_test 

memory_pool_test: memory_pool.o swap.o test.o logger.o
	$(CC) $(FLAGS) memory_pool.o swap.o test.o logger.o -o memory_pool_test 

check_result: check.o
	$(CC) $(FLAGS) check.o -o check_result

memory_manager.o: memory_manager.hpp memory_manager.cpp
	$(CC) $(FLAGS) -c memory_manager.cpp

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

rebuild: clean all

clean:
	rm -rf *.o *.bin

run:
	./memory_pool_test
	./check_result
