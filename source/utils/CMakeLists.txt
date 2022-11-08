file(GLOB CPPS *.cpp)

add_library(utils ${CPPS})

set_target_properties(utils PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-Werror"
)

