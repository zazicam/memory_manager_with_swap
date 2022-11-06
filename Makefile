CC=g++
FLAGS=-Wall -Werror -Wextra -Waddress -Warray-bounds=1 -Wbool-compare -Wbool-operation 
BUILD_DIR=./build
EXECUTABLE=memory_manager_test

all: $(EXECUTABLE) 

memory_manager_test: memory_manager.o memory_pool.o memory_block.o swap.o logger.o utils.o test.o 
	mv *.o $(BUILD_DIR) 
	cd $(BUILD_DIR);    \
	$(CC) $(FLAGS) memory_manager.o memory_pool.o memory_block.o swap.o utils.o logger.o test.o \
	-o $(EXECUTABLE)  

memory_manager.o: memory_manager.hpp memory_manager.cpp
	$(CC) $(FLAGS) -c memory_manager.cpp

utils.o: utils.hpp utils.cpp
	$(CC) $(FLAGS) -c utils.cpp

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
	rm -f $(BUILD_DIR)/$(EXECUTABLE) 

run:
	./$(BUILD_DIR)/$(EXECUTABLE) 50;  # Permission to allocate N Mb RAM
	bash ./check_result.sh
