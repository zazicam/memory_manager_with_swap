add_subdirectory(memory_manager)
add_subdirectory(utils)

add_executable(memory_manager_test test.cpp)

link_directories(
	memory_manager 
	utils
)

target_link_libraries(memory_manager_test 
	memory_manager 
	utils
)

set_target_properties(memory_manager_test PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror"
)

