cmake_minimum_required(VERSION 3.24)
project(
	test68k
	VERSION 0.1.0
	LANGUAGES C
)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors." ON)
option(ENABLE_SANITIZERS "" ON)

include("cmake/base.cmake")
include("cmake/warnings.cmake")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GEN_SCRIPT "${CMAKE_SOURCE_DIR}/src/main.py")
set(OUT_DIR "${CMAKE_BINARY_DIR}/generated")
set(WRAPPER_LIB $<TARGET_FILE:musashi_wrapper>)

add_subdirectory("vendor/Musashi")

add_library(musashi_wrapper SHARED
	"src/wrapper/wrapper.c"
)
set_default_warnings(musashi_wrapper)
enable_tools(musashi_wrapper)

target_compile_features(musashi_wrapper PRIVATE c_std_23)

target_include_directories(
	musashi_wrapper PRIVATE "${CMAKE_SOURCE_DIR}/src/wrapper")

target_link_libraries(musashi_wrapper PRIVATE musashi)

set_target_properties(
	musashi_wrapper
	PROPERTIES
		OUTPUT_NAME "musashi_wrapper"
		POSITION_INDEPENDENT_CODE ON
)

add_custom_target(run_generator
	DEPENDS musashi_wrapper
	COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
	COMMAND ${CMAKE_COMMAND} -E env
		"T68K_WRAPPER_LIB=${WRAPPER_LIB}"
		"LD_LIBRARY_PATH=$<TARGET_FILE_DIR:musashi_wrapper>:$ENV{LD_LIBRARY_PATH}"
		${Python3_EXECUTABLE} "${GEN_SCRIPT}" --out "${OUT_DIR}"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Running test generator"
	VERBATIM
)
