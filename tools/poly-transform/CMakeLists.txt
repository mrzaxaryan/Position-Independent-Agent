cmake_minimum_required(VERSION 3.20)
project(PolyTransform LANGUAGES C CXX)

# ── Static-link C++ runtime for portable binaries ────────────────────────────
option(STATIC_LINK "Statically link C++ runtime" OFF)
if(STATIC_LINK AND MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# =============================================================================
# Find LLVM
# =============================================================================
find_package(LLVM REQUIRED CONFIG)

message(STATUS "[poly-transform] Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "[poly-transform] LLVMConfig.cmake: ${LLVM_DIR}")
message(STATUS "[poly-transform] LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "[poly-transform] RTTI: ${LLVM_ENABLE_RTTI}")
message(STATUS "[poly-transform] Plugins: ${LLVM_ENABLE_PLUGINS}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# =============================================================================
# Determine build mode: always standalone
# =============================================================================
# The pass runs in the compile pipeline as a standalone tool step
# (clang -emit-llvm → poly-transform → clang -c), not as a pass plugin.
# This avoids conflicts with LTO which runs pass plugins at both compile
# and link time, causing symbol resolution failures.
if(TRUE)
	# ── Standalone executable mode ───────────────────────────────────────
	message(STATUS "[poly-transform] Building standalone executable")

	llvm_map_components_to_libnames(LLVM_LIBS
		core
		support
		option
		irreader
		bitwriter
		passes
		transformutils
	)
	message(STATUS "[poly-transform] LLVM libs: ${LLVM_LIBS}")

	add_executable(poly-transform
		PolyTransformOpt.cpp
		PolyTransformPass.cpp
	)
	target_compile_features(poly-transform PRIVATE cxx_std_17)
	target_compile_definitions(poly-transform PRIVATE POLY_TRANSFORM_STANDALONE)
	if(NOT LLVM_ENABLE_RTTI)
		target_compile_options(poly-transform PRIVATE -fno-rtti)
	endif()
	target_link_libraries(poly-transform PRIVATE ${LLVM_LIBS})

	if(STATIC_LINK)
		message(STATUS "[poly-transform] Static linking enabled")
		if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
			target_link_options(poly-transform PRIVATE -static-libgcc -static-libstdc++)
		elseif(APPLE)
			target_link_options(poly-transform PRIVATE -static-libstdc++)
		endif()
	endif()

else()
	# ── Loadable pass plugin mode ────────────────────────────────────────
	message(STATUS "[poly-transform] Building loadable pass plugin")

	add_library(PolyTransform MODULE
		PolyTransformPass.cpp
	)
	target_compile_features(PolyTransform PRIVATE cxx_std_17)

	if(WIN32)
		target_link_libraries(PolyTransform PRIVATE LLVM-C)
	endif()

	set_target_properties(PolyTransform PROPERTIES
		PREFIX ""
		OUTPUT_NAME "PolyTransform"
	)
endif()
