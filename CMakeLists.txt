cmake_minimum_required(VERSION 3.29)
project(
	test68k
	VERSION 0.1.0
	LANGUAGES CXX
)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors." ON)
option(ENABLE_SANITIZERS "Enable sanitizers" ON)
option(ENABLE_CCACHE "Enable CCache" ON)

include("cmake/base.cmake")
include("cmake/warnings.cmake")

add_executable(${PROJECT_NAME})
set_default_warnings(${PROJECT_NAME})
enable_tools(${PROJECT_NAME})

# Enable code sanitizers
if(ENABLE_SANITIZERS)
	target_compile_options(${PROJECT_NAME} PRIVATE ${ASAN_SANITIZER_FLAGS})
	target_link_options(${PROJECT_NAME} PRIVATE ${ASAN_SANITIZER_FLAGS})
endif()

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_20
)

target_sources(
	${PROJECT_NAME}
	PRIVATE
		"${CMAKE_SOURCE_DIR}/vendor/Moira/Moira.cpp"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDebugger.cpp"
		"${CMAKE_SOURCE_DIR}/src/main.cpp"
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		"${CMAKE_SOURCE_DIR}/vendor/Moira"
		"${CMAKE_SOURCE_DIR}/src"
)

set_source_files_properties(
	"${CMAKE_SOURCE_DIR}/vendor/Moira/Moira.cpp"
	"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDebugger.cpp"
	PROPERTIES
	COMPILE_OPTIONS "-w"
)
