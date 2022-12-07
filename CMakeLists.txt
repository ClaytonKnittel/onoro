cmake_minimum_required(VERSION 3.18)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message("Setting build type to 'RelWithDebInfo' as none was specified")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
		"RelWithDebInfo" "MinSizeRel")
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	find_program(LSB_RELEASE_EXEC lsb_release REQUIRED)
	execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
		OUTPUT_VARIABLE LINUX_DISTR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if("${LINUX_DISTR}" MATCHES "Ubuntu")
		list(APPEND LINK_LIBS "rt;pthread")
	endif()
else()
	set(LINUX_DISTR "")
endif()

project(onoro VERSION 0.1)


############################################################
# Options
############################################################

set(BUILD_SHARED_LIBS ON CACHE BOOL "When enabled, builds libonoro as a
	shared library, otherwise building as a static library")
set(ENABLE_TESTING OFF CACHE BOOL "When enabled, build all unit tests. The
	unit tests can then be run from within the build directory with
	'make run_tests'.")
set(ABSL_PROPAGATE_CXX_STD ON)


############################################################
# Dependencies
############################################################

find_package(Protobuf REQUIRED)
find_package(Python REQUIRED COMPONENTS Interpreter)


############################################################
# Build Rules
############################################################

file(GLOB_RECURSE CXX_SRC
	"${PROJECT_SOURCE_DIR}/src/*.cc"
)
file(GLOB_RECURSE PROTO_SRC
  "${PROJECT_SOURCE_DIR}/proto/*.proto"
)

set(TEST_CXX_SRC ${CXX_SRC})
list(FILTER TEST_CXX_SRC INCLUDE REGEX "test_[^/]+\.cc")
message("test src: ${TEST_CXX_SRC}")

list(FILTER CXX_SRC EXCLUDE REGEX "test_[^/]+\.cc")

# remove arch_test dir
list(FILTER CXX_SRC EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/src/arch_test/*")
list(FILTER PROTO_SRC EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/src/arch_test/*")

protobuf_generate_cpp(PROTO_GEN_SRCS PROTO_GEN_HDRS ${PROTO_SRC})

add_executable("onoro"
  ${CXX_SRC} ${PROTO_GEN_SRCS} ${PROT_GEN_HDRS}
)

set(TEST_CXX_EXES "")

# Add an executable for each test_*.cc file
foreach(TEST_CXX_SRC IN LISTS TEST_CXX_SRC)
  string(REGEX REPLACE "^.+test_([^/]+)\.cc" "test_\\1" EXE_NAME ${TEST_CXX_SRC})
  add_executable(${EXE_NAME}
    ${TEST_CXX_SRC} ${PROTO_GEN_SRCS} ${PROTO_GEN_HDRS}
  )
  list(APPEND TEST_CXX_EXES ${EXE_NAME})
endforeach()

############################################################
# Add submodules
############################################################

add_subdirectory(modules/abseil-cpp EXCLUDE_FROM_ALL)
add_subdirectory(modules/utils EXCLUDE_FROM_ALL)

set(ONORO_EXE "onoro")

foreach(EXE IN LISTS ONORO_EXE TEST_CXX_EXES)

  if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
    enable_language(ASM)
    if(CMAKE_ASM_COMPILER_WORKS)
      set(can_use_assembler TRUE)
    endif()
  endif()

  if(NOT can_use_assembler)
    message(SEND_ERROR "No assembler found")
  endif()

  target_compile_options(${EXE} PRIVATE
    $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:-Wpedantic -Wall -Wextra
    -Wno-unused-function -Wno-format -march=native -mtune=native>
  )
  target_compile_options(${EXE}
    PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-extra-semi -Wno-c99-extensions>
    PUBLIC
      $<$<COMPILE_LANGUAGE:CXX>:-Wno-nested-anon-types -Wno-ignored-qualifiers
      -Wno-gnu-anonymous-struct -Wno-unused-parameter -Wno-newline-eof
      -Wno-c++11-extensions -Wno-c++14-extensions -Wno-c++17-extensions
      -Wno-gnu-zero-variadic-macro-arguments -Wno-vla-extension -Wno-gcc-compat>
  )

  if("${CMAKE_BUILD_TYPE}" MATCHES "Debug" OR "${CMAKE_BUILD_TYPE}" MATCHES "RelWithDebInfo")
    target_compile_definitions(${EXE} PRIVATE DEBUG_BUILD)
  elseif("${CMAKE_BUILD_TYPE}" MATCHES "Release")
    target_compile_definitions(${EXE} PRIVATE RELEASE_BUILD)
  endif()

  include(CheckCXXCompilerFlag)
  CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

  if (NOT COMPILER_SUPPORTS_CXX17)
    message(SEND_ERROR "The compiler ${CMAKE_CXX_COMPILER} does not support "
      "c++17, please use a different compiler")
  endif()

  set_property(TARGET ${EXE} PROPERTY CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD 17)

  target_include_directories(${EXE}
    PRIVATE
      "/usr/local/include"
    PUBLIC
      $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${PROJECT_SOURCE_DIR}/include>
      $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${PROJECT_SOURCE_DIR}/modules/abseil-cpp>
      $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${PROJECT_SOURCE_DIR}/modules/utils/include>
      $<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${CMAKE_CURRENT_BINARY_DIR}>
  )

  target_link_libraries(${EXE}
    PUBLIC
      "${LINK_LIBS}" absl::flat_hash_map absl::statusor absl::str_format utils ${Protobuf_LIBRARIES}
  )

endforeach()

############################################################
# Python lib
############################################################

protobuf_generate_python(PROTO_GEN_PY ${PROTO_SRC})

add_custom_target("py"
  DEPENDS ${PROTO_GEN_PY}
  COMMENT "Run python version of onoro"
)

add_custom_command(
  TARGET "py"
  COMMAND PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} python3 ${CMAKE_SOURCE_DIR}/pylib/init.py
)

