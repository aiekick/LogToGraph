set(SQLITE3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/sqlite3)
set(SQLITE3_LIBRARIES sqlite3)

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
	add_library(sqlite3 
		${SQLITE3_INCLUDE_DIR}/src/sqlite3.c
		${SQLITE3_INCLUDE_DIR}/include/sqlite3.h
		${SQLITE3_INCLUDE_DIR}/include/sqlite3ext.h
		${SQLITE3_INCLUDE_DIR}/sqlite3.hpp
	)
	if (WIN32)
		target_compile_definitions(sqlite3 PUBLIC sqlite3_EXPORTS PRIVATE BUILD_SQLITE_SHARED_LIBS)
	endif()
	set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
	set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
	add_library(sqlite3 STATIC
		${SQLITE3_INCLUDE_DIR}/src/sqlite3.c
		${SQLITE3_INCLUDE_DIR}/include/sqlite3.h
		${SQLITE3_INCLUDE_DIR}/include/sqlite3ext.h
		${SQLITE3_INCLUDE_DIR}/sqlite3.hpp
	)
endif()

if(USE_SHARED_LIBS)
	set_target_properties(sqlite3 PROPERTIES FOLDER 3rdparty/Shared)
else()
	set_target_properties(sqlite3 PROPERTIES FOLDER 3rdparty/Static)
endif()

set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)

set_target_properties(sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(sqlite3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

install(TARGETS sqlite3 RUNTIME DESTINATION / COMPONENT APP_LIBS_SQLITE3)

include_directories(${SQLITE3_INCLUDE_DIR})
