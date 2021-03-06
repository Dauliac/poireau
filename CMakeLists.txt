# cmake -DLLVM_DIR="/usr/lib/llvm-10/lib/cmake/llvm" -DClang_DIR="/usr/lib/llvm-10/lib/cmake/clang" ..

cmake_minimum_required( VERSION 3.16 )

project( Raisin
	LANGUAGES CXX
	VERSION 0.0.1
	DESCRIPTION "Produce CFG from a C++ source code"
	)

list( APPEND CMAKE_MODULE_PATH
	"${CMAKE_SOURCE_DIR}/cmake"
	)

if( NOT CMAKE_BUILD_TYPE )
	set( CMAKE_BUILD_TYPE "Release" )
endif()
message( STATUS "Build type: ${CMAKE_BUILD_TYPE}" )

option( SONAR "The coverage target will produce SonarQube reports instead of console / HTML" OFF )

if( CMAKE_BUILD_TYPE MATCHES Coverage )
	if( CMAKE_CXX_COMPILER_ID MATCHES GNU )
		find_program( GCOV NAMES gcov )
		if( GCOV )
			message( STATUS "Found gcov: ${GCOV}" )
		else()
			message( ERROR "Not found gcov: install it" )
		endif()

		if( SONAR )
			find_program( GCOVR NAMES gcovr )
			if( GCOVR )
				message( STATUS "Found gcovr: ${GCOVR}" )
			else()
				message( ERROR "Not found gcovr: install it" )
			endif()
		else()
			find_program( LCOV NAMES lcov )
			if( LCOV )
				message( STATUS "Found lcov: ${LCOV}" )
			else()
				message( ERROR "Not found lcov: install it" )
			endif()

			find_program( GENHTML NAMES genhtml )
			if( GENHTML )
				message( STATUS "Found genhtml: ${GENHTML}" )
			else()
				message( ERROR "Not found genhtml: install it" )
			endif()
		endif()

	elseif( CMAKE_CXX_COMPILER_ID MATCHES Clang )

		find_program( LLVM_COV NAMES llvm-cov )
		if( LLVM_COV )
			message( STATUS "Found llvm-cov: ${LLVM_COV}" )
		else()
			message( ERROR "Not found llvm-cov: install it" )
		endif()

		find_program( LLVM_PROFDATA NAMES llvm-profdata )
		if( LLVM_PROFDATA )
			message( STATUS "Found llvm-profdata: ${LLVM_PROFDATA}" )
		else()
			message( ERROR "Not found llvm-profdata: install it" )
		endif()
	endif()

	find_package( Catch2 3.0.0 REQUIRED )
	if( Catch2_FOUND )
		message( STATUS "Found Catch2: ${Catch2_VERSION}" )
	endif()

else()

	find_package( Catch2 3.0.0 QUIET )
	if( Catch2_FOUND )
		message( STATUS "Found Catch2: ${Catch2_VERSION}" )
	else()
		message( STATUS "Not found Catch2: unittest target disabled" )
	endif()
endif()

find_package( Clang REQUIRED )

message( STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}" )
message( STATUS "Using ClangConfig.cmake in: ${Clang_DIR}" )
message( STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}" )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON )

add_subdirectory( "${CMAKE_SOURCE_DIR}/icon" )
add_subdirectory( "${CMAKE_SOURCE_DIR}/src" )
add_subdirectory( "${CMAKE_SOURCE_DIR}/test" )
