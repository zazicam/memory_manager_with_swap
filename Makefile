CC=g++
FLAGS=-Wall -Werror -Wextra -Waddress -Warray-bounds=1 -Wbool-compare -Wbool-operation 
BUILD_DIR=./build

all: memory_manager_test 

memory_manager_test: memory_manager.o memory_pool.o memory_block.o swap.o logger.o test.o 
	mv *.o $(BUILD_DIR) 
	cd $(BUILD_DIR);    \
	$(CC) $(FLAGS) memory_manager.o memory_pool.o memory_block.o swap.o test.o logger.o \
	-o memory_manager_test 

memory_manager.o: memory_manager.hpp memory_manager.cpp
	$(CC) $(FLAGS) -c memory_manager.cpp

memory_pool.o: memory_pool.hpp memory_pool.cpp
	$(CC) $(FLAGS) -c memory_pool.cpp

memory_block.o: memory_block.hpp memory_block.cpp
	$(CC) $(FLAGS) -c memory_block.cpp

swap.o: swap.hpp swap.cpp
	$(CC) $(FLAGS) -c swap.cpp

test.o: test.cpp
	$(CC) $(FLAGS) -c test.cpp

logger.o: logger.cpp logger.hpp
	$(CC) $(FLAGS) -c logger.cpp

build: all

rebuild: clean all

clean:
	rm -rf $(BUILD_DIR)/*.o 
	rm -rf $(BUILD_DIR)/*.bin 

run:
	./$(BUILD_DIR)/memory_manager_test 100;  # Permission to allocate 100 Mb RAM
	bash ./check_result.sh
