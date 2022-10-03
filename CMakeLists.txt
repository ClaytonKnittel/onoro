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


############################################################
# Build Rules
############################################################

file(GLOB_RECURSE CXX_SRC
	"${PROJECT_SOURCE_DIR}/src/*.cc"
)
file(GLOB_RECURSE ASM_SRC
	"${PROJECT_SOURCE_DIR}/src/*.s"
)

# remove arch_test dir
list(FILTER CXX_SRC EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/src/arch_test/*")
list(FILTER ASM_SRC EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/src/arch_test/*")

add_executable(onoro
  ${CXX_SRC} ${ASM_SRC}
)

if("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "x86_64")
	enable_language(ASM)
	if(CMAKE_ASM_COMPILER_WORKS)
		set(can_use_assembler TRUE)
	endif()
endif()

if(NOT can_use_assembler)
	message(SEND_ERROR "No assembler found")
endif()

target_compile_options(onoro PRIVATE
	$<$<NOT:$<COMPILE_LANGUAGE:ASM>>:-Wpedantic -Wall -Wextra
  -Wno-unused-function -Wno-format -march=native -mtune=native>
)
target_compile_options(onoro
	PRIVATE
		$<$<COMPILE_LANGUAGE:CXX>:-Wno-extra-semi -Wno-c99-extensions>
	PUBLIC
		$<$<COMPILE_LANGUAGE:CXX>:-Wno-nested-anon-types -Wno-ignored-qualifiers
		-Wno-gnu-anonymous-struct -Wno-unused-parameter -Wno-newline-eof
		-Wno-c++11-extensions -Wno-c++14-extensions -Wno-c++17-extensions
    -Wno-gnu-zero-variadic-macro-arguments -Wno-vla-extension>
)

if("${CMAKE_ASM_COMPILER_ID}" MATCHES "GNU")
	set(ASM_OPTIONS -Wa,--64 -Wa,-msyntax=intel -Wa,-mnaked-reg)
elseif("${CMAKE_ASM_COMPILER_ID}" MATCHES "Clang")
	set(ASM_OPTIONS -masm=intel)
else()
	message(SEND_ERROR "Unsupported assembler (${CMAKE_ASM_COMPILER_ID})")
endif()

target_compile_options(onoro PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${ASM_OPTIONS}>)

if (${ENABLE_TESTING})
	target_compile_definitions(onoro PRIVATE DO_TESTING)
endif()

if("${CMAKE_BUILD_TYPE}" MATCHES "Debug" OR "${CMAKE_BUILD_TYPE}" MATCHES "RelWithDebInfo")
	target_compile_definitions(onoro PRIVATE DEBUG_BUILD)
elseif("${CMAKE_BUILD_TYPE}" MATCHES "Release")
	target_compile_definitions(onoro PRIVATE RELEASE_BUILD)
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)

if (NOT COMPILER_SUPPORTS_CXX17)
	message(SEND_ERROR "The compiler ${CMAKE_CXX_COMPILER} does not support "
		"c++17, please use a different compiler")
endif()

set_property(TARGET onoro PROPERTY CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(modules/abseil-cpp EXCLUDE_FROM_ALL)

target_include_directories(onoro
	PRIVATE
		"/usr/local/include"
	PUBLIC
		$<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${PROJECT_SOURCE_DIR}/include>
		$<$<NOT:$<COMPILE_LANGUAGE:ASM>>:${PROJECT_SOURCE_DIR}/modules/abseil-cpp>
)

target_link_libraries(onoro
	PUBLIC
		"${LINK_LIBS}" absl::flat_hash_map
)

if (${ENABLE_TESTING})
	find_library(check check REQUIRED)

	target_link_directories(onoro
		PUBLIC "/usr/local/lib"
	)

	target_link_libraries(onoro
		PUBLIC
			check
	)

	add_subdirectory(test)
endif()


############################################################
# Installation Rules
############################################################

# Library
if(${BUILD_SHARED_LIBS})
	install(TARGETS onoro LIBRARY DESTINATION lib)
else()
	install(TARGETS onoro ARCHIVE DESTINATION lib)
endif()

# Header files
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION include)

# make uninstall
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
    TARGET "uninstall"
    POST_BUILD
    COMMENT "Uninstall files with install_manifest.txt"
    COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in
            install_manifest.txt to be uninstalled!
)

