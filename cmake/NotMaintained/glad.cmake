FetchContent_Declare(
	glad
	GIT_REPOSITORY	https://github.com/Dav1dde/glad.git
	GIT_TAG			5bf3eda6da606324999775b88a90ed572202be93 # branch c
	SOURCE_DIR		${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/glad
	GIT_PROGRESS	true
	GIT_SHALLOW		true
)

FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
	FetchContent_Populate(glad)
	
	if(USE_SHARED_LIBS)
		set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
		set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
	else()
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
		set(LLVM_USE_CRT_DEBUG MTd CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_MINSIZEREL MT CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELEASE MT CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELWITHDEBINFO MT CACHE STRING "" FORCE)
	endif()
	
	if(NOT CMAKE_DEBUG_POSTFIX)
	  set(CMAKE_DEBUG_POSTFIX _debug)
	endif()
	if(NOT CMAKE_RELEASE_POSTFIX)
	  set(CMAKE_RELEASE_POSTFIX)
	endif()
	if(NOT CMAKE_MINSIZEREL_POSTFIX)
	  set(CMAKE_MINSIZEREL_POSTFIX _minsizerel)
	endif()
	if(NOT CMAKE_RELWITHDEBINFO_POSTFIX)
	  set(CMAKE_RELWITHDEBINFO_POSTFIX _reldeb)
	endif()

	if (BUILD_SHARED_LIBS)
		set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "" FORCE)
		add_library(glad 
			${glad_SOURCE_DIR}/src/glad.c
			${glad_SOURCE_DIR}/include/glad/glad.h
		)
		if (WIN32)
			# https://github.com/Dav1dde/glad/issues/113#issuecomment-323520027
			target_compile_definitions(glad PUBLIC GLAD_GLAPI_EXPORT PRIVATE GLAD_GLAPI_EXPORT_BUILD)
		endif()
		set_target_properties(glad PROPERTIES POSITION_INDEPENDENT_CODE ON)
	else()
		set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
		add_library(glad STATIC
			${glad_SOURCE_DIR}/src/glad.c
			${glad_SOURCE_DIR}/include/glad/glad.h
		)
	endif()

	if(USE_SHARED_LIBS)
		set_target_properties(glad PROPERTIES FOLDER 3rdparty/Shared)
	else()
		set_target_properties(glad PROPERTIES FOLDER 3rdparty/Static)
	endif()
	
	set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)

	set_target_properties(glad PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
	set_target_properties(glad PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
	set_target_properties(glad PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
	set_target_properties(glad PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
	set_target_properties(glad PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

	set(GLAD_INCLUDE_DIR ${glad_SOURCE_DIR}/include)
	set(GLAD_LIBRARIES glad)
	
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(glad PRIVATE -Wno-everything) # disable all warnings, since im not maintaining this lib
	endif()

	include_directories(${GLAD_INCLUDE_DIR})

	if(NOT WIN32)
		set(GLAD_LIBRARIES ${GLAD_LIBRARIES} dl)
	endif()
endif()
