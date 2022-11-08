file(GLOB CPPS *.cpp)

add_library(memory_manager ${CPPS})

set_target_properties(memory_manager PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror"
)
