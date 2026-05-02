cmake_minimum_required(VERSION 3.24)
project(
	musashi
	VERSION 0.1.0
	LANGUAGES C
)

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

add_library(${PROJECT_NAME} STATIC
	"${MUSASHI_DIR}/m68kcpu.c"
	"${MUSASHI_DIR}/softfloat/softfloat.c"
	"${MUSASHI_GEN_C}"
)
add_dependencies(${PROJECT_NAME} ${MUSASHI_CODEGEN_EXE})

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
		"${MUSASHI_DIR}"
		"${MUSASHI_DIR}/softfloat"
)

target_link_libraries(${PROJECT_NAME} PUBLIC m)
