cmake_minimum_required(VERSION 3.0)

project(memory_manager_test)

file(GLOB CPPS *.cpp)

add_executable(${PROJECT_NAME} ${CPPS})

set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror"
)


