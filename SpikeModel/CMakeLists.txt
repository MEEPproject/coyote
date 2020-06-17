cmake_minimum_required (VERSION 3.14)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
project(SPIKE_MODEL)

set (GEN_DEBUG_INFO ON CACHE BOOL "Genearate debug info in compile & link -g")


set (SPARTA_BASE ${CMAKE_CURRENT_SOURCE_DIR}/../map/sparta)

if(DEFINED $ENV{SPARTA_PATH})
	set(SPARTA_BASE $ENV{SPARTA_PATH})
ENDIF()

include (${SPARTA_BASE}/cmake/sparta-config.cmake)

# Add RapidJSON
include_directories (SYSTEM ${RAPIDJSON_INCLUDE_DIRS})

# Add YAML CPP
include_directories (${YAML_CPP_INCLUDE_DIR})

# Add Boost (not typical system install)
include_directories (SYSTEM ${Boost_INCLUDE_DIRS})

# Add HDF5 (not typical system install)
include_directories (SYSTEM ${HDF5_C_INCLUDE_DIRS})
include_directories (SYSTEM ${HDF5_INCLUDE_DIRS})


#
# Debug and Release are cmake internal build types.  Release will add
# the appropriate optimization flags and turn on NDEBUG automatically.
# Debug will build unoptimized and allow asserts.
#
# SPARTA cmake will _always_ have debug symbols regardless of the
# build type.
#
set (SPARTA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../map/sparta/release/)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	message ("-- Building SpikeModel in Debug mode")
	set (SPARTA_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../map/sparta/debug/)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
	message ("-- Building SpikeModel in Release mode")
else ()
  message (FATAL_ERROR "Unknown CMAKE_BUILD_TYPE.  See README.md OR type 'make' at the SPARTA root")
endif ()

# Use ccache if we've got it.
find_program (CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
  set_property (GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  message ("-- Using ccache")
endif ()

add_compile_options(-Werror -fPIC
  -Wall -Wextra -Winline -Winit-self -Wno-unused-function
  -Wuninitialized -Wno-sequence-point -Wno-inline -Wno-unknown-pragmas
  -Woverloaded-virtual -Wno-unused-parameter -Wno-missing-field-initializers)

if (GEN_DEBUG_INFO)
  message ("-- Building with debug info -g")
  add_compile_options ( -g )
  add_link_options( -g )
else ()
  message ("-- Building without debug info -g")
endif ()

if (ENABLE_SANITIZERS)
    message ("-- Building with address and undefined behavior sanitizers")
    add_compile_options(-fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined)
    set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
endif ()

#
# If we're using CONDA, we might be using the one suggested for
# Sparta.  Need to use the llvm-ar found in the conda package to
# prevent irritating linker issues
#
if (USING_CONDA)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    unset(LLVM_AR)
    unset(LLVM_AR CACHE)
    find_program(LLVM_AR "llvm-ar")
    if (NOT LLVM_AR)
      unset(LLVM_AR)
      unset(LLVM_AR CACHE)
      find_program(LLVM_AR "llvm-ar-9")
      if (NOT LLVM_AR)
        message(FATAL_ERROR "llvm-ar is needed to link trace_tools on this system")
      else()
        SET(CMAKE_AR "llvm-ar-9")
      endif()
    else()
      SET(CMAKE_AR "llvm-ar")
    endif()
  endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # using Clang
  # -stdlib=libc++
  add_compile_options(-Wpedantic -Wno-gnu-zero-variadic-macro-arguments)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # using GCC
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
endif ()

# Add a custom CMake Modules directory
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# CppCheck Support
set (CPPCHECK_XML_OUTPUT "${PROJECT_BINARY_DIR}/analysis/cppcheck/cppcheck_analysis.xml")
set (CPPCHECK_EXCLUDES
  ${CMAKE_BINARY_DIR}
  ${SPARTA_BASE}/test
)
find_package (Cppcheck)

#add_library (sparta ${SourceCppFiles})
#set (SPARTA_STATIC_LIBS ${PROJECT_BINARY_DIR}/libsparta.a)

add_library(sparta SHARED IMPORTED)
set_target_properties(sparta PROPERTIES IMPORTED_LOCATION ${SPARTA_LIB}/libsparta.a)

add_library(simdb SHARED IMPORTED)
set_target_properties(simdb PROPERTIES IMPORTED_LOCATION ${SPARTA_LIB}/simdb/libsimdb.a)

include (${SPARTA_BASE}/example/ExamplesMacros.cmake)


set(SPIKE_PATH ${SPARTA_BASE}/../../riscv-isa-sim)
if(DEFINED $ENV{SPIKE_PATH})
	set(SPIKE_PATH, $ENV{SPIKE_PATH})
ENDIF()

message("The sparta lib is "${SPARTA_LIB})

add_executable(spike_model
  src/main.cpp
  src/SpikeModel.cpp
  src/CPUFactory.cpp
  src/CPUTopology.cpp
  src/CPU.cpp
  src/Core.cpp
  src/L2Cache.cpp
  src/NoC.cpp
  ${SPIKE_PATH}/spike_main/spike_wrapper.cc
  )

sparta_application(spike_model)

target_link_libraries(spike_model ${SPIKE_PATH}/build/libspike_main.a  ${SPIKE_PATH}/build/libriscv.a  ${SPIKE_PATH}/build/libsoftfloat.a  ${SPIKE_PATH}/build/libfesvr.a -ldl)
include_directories(SYSTEM ${SPIKE_PATH}/riscv/ ${SPIKE_PATH}/build ${SPIKE_PATH}/softfloat ${SPIKE_PATH}/ ${SPIKE_PATH}/spike_main/ ./src/ ${SPARTA_BASE} ${SPARTA_BASE}/simdb/include)
