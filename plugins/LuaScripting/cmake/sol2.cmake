set(SOL2_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/sol2)
set(SOL2_INCLUDE_DIR ${SOL2_SOURCE_DIR}/include)
set(SOL2_LIBRARIES sol2)

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

set(SOL2_LUA_VERSION "LuaJIT 2.1.0-beta3" CACHE STRING "")
set(SOL2_BUILD_LUA TRUE CACHE BOOL "")
set(SOL2_PLATFORM "x64" CACHE STRING "")
set(SOL2_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(SOL2_TESTS_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_LUA_AS_DLL ON CACHE BOOL "" FORCE)
	
add_subdirectory(${SOL2_SOURCE_DIR} EXCLUDE_FROM_ALL)
  
set_target_properties(sol2 PROPERTIES FOLDER 3rdparty)

if(USE_SHARED_LIBS)
	set_target_properties(sol2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
	set_target_properties(sol2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
	set_target_properties(sol2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
	set_target_properties(sol2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")
endif()

