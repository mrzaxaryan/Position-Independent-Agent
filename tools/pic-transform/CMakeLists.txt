cmake_minimum_required(VERSION 3.20)
project(PICTransform LANGUAGES C CXX)

# ── Static-link C++ runtime for portable binaries ────────────────────────────
option(STATIC_LINK "Statically link C++ runtime" OFF)
if(STATIC_LINK AND MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# =============================================================================
# Find LLVM
# =============================================================================
# Point LLVM_DIR to the cmake config directory, e.g.:
#   cmake -B build -DLLVM_DIR=/usr/lib/llvm-22/lib/cmake/llvm
#   cmake -B build -DLLVM_DIR="D:/git/clang+llvm-22.x/lib/cmake/llvm"
find_package(LLVM REQUIRED CONFIG)

message(STATUS "[pic-transform] Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "[pic-transform] LLVMConfig.cmake: ${LLVM_DIR}")
message(STATUS "[pic-transform] LLVM include dirs: ${LLVM_INCLUDE_DIRS}")
message(STATUS "[pic-transform] RTTI: ${LLVM_ENABLE_RTTI}")
message(STATUS "[pic-transform] Plugins: ${LLVM_ENABLE_PLUGINS}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# =============================================================================
# Determine build mode: plugin vs standalone
# =============================================================================
# Also verify PassPlugin.h exists (some distro packages set LLVM_ENABLE_PLUGINS
# but don't ship the header).
if(LLVM_ENABLE_PLUGINS)
	find_file(_PASS_PLUGIN_H "llvm/Passes/PassPlugin.h" PATHS ${LLVM_INCLUDE_DIRS} NO_DEFAULT_PATH)
	if(NOT _PASS_PLUGIN_H)
		message(STATUS "[pic-transform] PassPlugin.h not found — falling back to standalone tool")
		set(LLVM_ENABLE_PLUGINS OFF)
	endif()
endif()

if(NOT LLVM_ENABLE_PLUGINS)
	# ── Standalone executable mode ───────────────────────────────────────
	message(STATUS "[pic-transform] Building standalone executable")

	llvm_map_components_to_libnames(LLVM_LIBS
		core
		support
		option
		irreader
		bitwriter
		passes
		transformutils
	)
	message(STATUS "[pic-transform] LLVM libs: ${LLVM_LIBS}")

	add_executable(pic-transform
		PICTransformOpt.cpp
		PICTransformPass.cpp
	)
	target_compile_features(pic-transform PRIVATE cxx_std_17)
	target_compile_definitions(pic-transform PRIVATE PIC_TRANSFORM_STANDALONE)
	if(NOT LLVM_ENABLE_RTTI)
		target_compile_options(pic-transform PRIVATE -fno-rtti)
	endif()
	target_link_libraries(pic-transform PRIVATE ${LLVM_LIBS})

	if(STATIC_LINK)
		message(STATUS "[pic-transform] Static linking enabled")
		if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
			target_link_options(pic-transform PRIVATE -static-libgcc -static-libstdc++)
		elseif(APPLE)
			target_link_options(pic-transform PRIVATE -static-libstdc++)
		endif()
	endif()

else()
	# ── Loadable pass plugin mode ────────────────────────────────────────
	message(STATUS "[pic-transform] Building loadable pass plugin")

	add_library(PICTransform MODULE
		PICTransformPass.cpp
	)
	target_compile_features(PICTransform PRIVATE cxx_std_17)

	if(WIN32)
		target_link_libraries(PICTransform PRIVATE LLVM-C)
	endif()

	set_target_properties(PICTransform PROPERTIES
		PREFIX ""
		OUTPUT_NAME "PICTransform"
	)
endif()
