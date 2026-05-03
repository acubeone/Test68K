cmake_minimum_required(VERSION 3.24)
project(
	test68k
	VERSION 0.1.0
	LANGUAGES C
)

include("cmake/base.cmake")
include("cmake/warnings.cmake")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GEN_SCRIPT "${CMAKE_SOURCE_DIR}/src/main.py")
set(OUT_DIR "${CMAKE_BINARY_DIR}/generated")
set(WRAPPER_LIB $<TARGET_FILE:musashi_wrapper>)
set(MUSASHI_DIR "${CMAKE_SOURCE_DIR}/vendor/Musashi")
set(MUSASHI_GEN_C "${MUSASHI_DIR}/m68kops.c")
set(MUSASHI_GEN_H "${MUSASHI_DIR}/m68kops.h")
set(MUSASHI_CODEGEN_EXE "${PROJECT_NAME}_codegen")
set(M68KMAKE m68kmake)

add_executable(${M68KMAKE} "${MUSASHI_DIR}/m68kmake.c")

add_custom_command(
	OUTPUT "${MUSASHI_GEN_C}" "${MUSASHI_GEN_H}"
	COMMAND $<TARGET_FILE:m68kmake>
	WORKING_DIRECTORY "${MUSASHI_DIR}"
	DEPENDS m68kmake "${MUSASHI_DIR}/m68k_in.c"
	COMMENT "Generating Musashi opcode tables..."
	VERBATIM
)
add_custom_target(
	${MUSASHI_CODEGEN_EXE}
	DEPENDS
		"${MUSASHI_GEN_C}"
		"${MUSASHI_GEN_H}"
)

add_library(musashi_wrapper SHARED
	"src/wrapper/wrapper.c"

	"${MUSASHI_DIR}/m68kcpu.c"
	"${MUSASHI_DIR}/m68kdasm.c"
	"${MUSASHI_DIR}/softfloat/softfloat.c"
	"${MUSASHI_GEN_C}"
)
add_dependencies(musashi_wrapper ${MUSASHI_CODEGEN_EXE})
set_default_warnings(musashi_wrapper)
enable_tools(musashi_wrapper)

target_compile_features(musashi_wrapper PRIVATE c_std_23)


if(USING_MSVC)
	target_compile_options(musashi_wrapper PRIVATE
		/FI"${CMAKE_SOURCE_DIR}/src/wrapper/m68kconf.h")
else()
	target_compile_options(musashi_wrapper PRIVATE
		-include "${CMAKE_SOURCE_DIR}/src/wrapper/m68kconf.h")
endif()



target_include_directories(
	musashi_wrapper
	PUBLIC
		"${CMAKE_SOURCE_DIR}/src/wrapper"
		"${MUSASHI_DIR}"
		"${MUSASHI_DIR}/softfloat"
)

target_link_libraries(musashi_wrapper PUBLIC m)

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
