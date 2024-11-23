FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG			0.9.9.8 # commpilation error with later versions
	SOURCE_DIR		${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/glm
	GIT_PROGRESS	true
	GIT_SHALLOW		true
)

FetchContent_GetProperties(glm)
if(NOT glm_POPULATED)
	FetchContent_Populate(glm)
	
	add_definitions(-DGLM_FORCE_SILENT_WARNINGS)
	
	if(USE_SHARED_LIBS)
		set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
		set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
		set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
	else()
		add_definitions(-DCURL_STATICLIB)
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
	
	## EXCLUDE_FROM_ALL reject install for this target
	add_subdirectory(${glm_SOURCE_DIR} EXCLUDE_FROM_ALL)
	
	set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR})
	include_directories(${GLM_INCLUDE_DIR}/glm)

	if(USE_SHARED_LIBS)
		set_target_properties(glm_shared PROPERTIES FOLDER 3rdparty/Shared)
		set_target_properties(glm_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
		set_target_properties(glm_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
		set_target_properties(glm_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
		set_target_properties(glm_shared PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")
		set(GLM_LIBRARIES glm_shared)
	else()
		set_target_properties(glm PROPERTIES FOLDER 3rdparty/Static)
		set(GLM_LIBRARIES glm)
	endif()
	
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(glm INTERFACE -Wno-everything) # disable all warnings, since im not maintaining this lib
	endif()
endif()
