add_library(moira STATIC)

target_compile_features(moira PRIVATE cxx_std_23)

target_include_directories(
	moira
	SYSTEM PUBLIC
		"${CMAKE_SOURCE_DIR}/vendor"
		"${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(
	moira
	PRIVATE
		"${CMAKE_SOURCE_DIR}/vendor/Moira/Moira.cpp"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDebugger.cpp"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraALU_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraConfig.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDasm_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDasm.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDataflow_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraExceptions_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraExec_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraExecMMU_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraInit_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraMacros.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/StrWriter_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraALU.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDasmFPU_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDasmMMU_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDataflow.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraDebugger.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraExceptions.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraExecFPU_cpp.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/Moira.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraInit.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/MoiraTypes.h"
		"${CMAKE_SOURCE_DIR}/vendor/Moira/StrWriter.h"
)

target_compile_definitions(
	moira
	PRIVATE
		"EMULATE_ADDRESS_ERROR=1"
		"EMULATE_FC=1"
		"ENABLE_DASM=1"
)

target_compile_options(
	moira
	PRIVATE
	$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-w>
	$<$<CXX_COMPILER_ID:MSVC>:/w>
)
