cmake_minimum_required(VERSION 4.2)
project(
	test68k
	VERSION 0.1.0
	LANGUAGES C
)

include("cmake/base.cmake")
include("cmake/warnings.cmake")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CACHE{TCS_GEN_SCRIPT} TYPE FILEPATH
	HELP "Generator python script" VALUE "${CMAKE_SOURCE_DIR}/src/main.py")
set(CACHE{TCS_OUTPUT_DIR} TYPE PATH
	HELP "Directory for output files" VALUE "${CMAKE_BINARY_DIR}/generated/")
set(CACHE{TCS_MAX_TESTS} TYPE STRING
	HELP "How many generated tests for each instruction" VALUE "16")
set(CACHE{TCS_DEFAULT_SEED} TYPE STRING
	HELP "Default random seed" VALUE "0")
set(CACHE{TCS_CPU_MODEL} TYPE STRING
	HELP "" VALUE "m68000")
set_property(CACHE TCS_CPU_MODEL PROPERTY STRINGS "m68000;m68010")

if(NOT $CACHE{TCS_MAX_TESTS} MATCHES "^[1-9][0-9]*$")
	message(FATAL_ERROR "TCS_MAX_TESTS must be a positive integer! Got: '${TCS_MAX_TESTS}'")
endif()

if(NOT $CACHE{TCS_DEFAULT_SEED} MATCHES "^[0-9]*$")
	message(FATAL_ERROR "TCS_DEFAULT_SEED must be an integer! Got: '${TCS_DEFAULT_SEED}'")
endif()

set(_tcs_musashi musashi_wrapper)
set(_tcs_wrapper_lib $<TARGET_FILE:${_tcs_musashi}>)
set(_tcs_musashi_dir "${CMAKE_SOURCE_DIR}/vendor/Musashi")
set(_tcs_musashi_gen_c "${_tcs_musashi_dir}/m68kops.c")
set(_tcs_musashi_gen_h "${_tcs_musashi_dir}/m68kops.h")
set(_tcs_musashi_codegen_exe "${PROJECT_NAME}_codegen")
set(_tcs_m68kmake m68kmake)

add_executable(${_tcs_m68kmake} "${_tcs_musashi_dir}/m68kmake.c")

add_custom_command(
	OUTPUT "${_tcs_musashi_gen_c}" "${_tcs_musashi_gen_h}"
	COMMAND $<TARGET_FILE:${_tcs_m68kmake}>
	WORKING_DIRECTORY "${_tcs_musashi_dir}"
	DEPENDS m68kmake "${_tcs_musashi_dir}/m68k_in.c"
	COMMENT "Generating Musashi opcode tables..."
	VERBATIM
)
add_custom_target(
	${_tcs_musashi_codegen_exe}
	DEPENDS
		"${_tcs_musashi_gen_c}"
		"${_tcs_musashi_gen_h}"
)

add_library(${_tcs_musashi} SHARED
	"src/wrapper/wrapper.c"

	"${_tcs_musashi_dir}/m68kcpu.c"
	"${_tcs_musashi_dir}/m68kdasm.c"
	"${_tcs_musashi_dir}/softfloat/softfloat.c"
	"${_tcs_musashi_gen_c}"
)
add_dependencies(${_tcs_musashi} ${_tcs_musashi_codegen_exe})
set_default_warnings(${_tcs_musashi})
enable_tools(${_tcs_musashi})

target_compile_features(${_tcs_musashi} PRIVATE c_std_23)
target_link_libraries(${_tcs_musashi} PUBLIC m)

target_include_directories(
	${_tcs_musashi}
	PUBLIC
		"${CMAKE_SOURCE_DIR}/src/wrapper"
		"${_tcs_musashi_dir}"
		"${_tcs_musashi_dir}/softfloat"
)

set_target_properties(
	${_tcs_musashi}
	PROPERTIES
		OUTPUT_NAME "musashi_wrapper"
		POSITION_INDEPENDENT_CODE ON
)

add_custom_target(run_generator
	DEPENDS musashi_wrapper
	COMMAND ${CMAKE_COMMAND} -E make_directory "$CACHE{TCS_OUTPUT_DIR}"
	COMMAND ${CMAKE_COMMAND} -E env
		"T68K_WRAPPER_LIB=${_tcs_wrapper_lib}"
		"LD_LIBRARY_PATH=$<TARGET_FILE_DIR:musashi_wrapper>:$ENV{LD_LIBRARY_PATH}"
		${Python3_EXECUTABLE} "$CACHE{TCS_GEN_SCRIPT}"
		--out "$CACHE{TCS_OUTPUT_DIR}"
		--max-tests "$CACHE{TCS_MAX_TESTS}"
		--seed "$CACHE{TCS_DEFAULT_SEED}"
		--model "$CACHE{TCS_CPU_MODEL}"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Running test generator"
	VERBATIM
)
