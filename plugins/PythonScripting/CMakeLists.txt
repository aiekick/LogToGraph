cmake_minimum_required(VERSION 3.1)

set(PROJECT PythonScripting)

enable_language(C CXX)

project(${PROJECT} CXX)

option(PLUGIN_ENABLE_PYTHON_SCRIPTING "Enable the plugin Python Scripting" ON)

if (WIN32 AND PLUGIN_ENABLE_PYTHON_SCRIPTING)

set(LLVM_USE_CRT_DEBUG MDd CACHE STRING "" FORCE)
set(LLVM_USE_CRT_MINSIZEREL MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELEASE MD CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELWITHDEBINFO MD CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DUSE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS) ## for the resize imgui issue when we have child glfw windows

if (uninstall)
	set_target_properties(uninstall PROPERTIES FOLDER "CmakeTargets")
endif()

if(UNIX)
    add_definitions(-DUNIX)
elseif(WIN32)
    add_definitions(-DWIN32)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 ")
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DMSVC)
	add_definitions(-DNOMINMAX) ## for avoid issue regarding std::max and std::min
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wdeprecated-declarations -Wunused-parameter -Wno-unknown-pragmas")
endif ()

include(cmake/python.cmake)

#find_package(Python COMPONENTS Interpreter Development)

file(GLOB_RECURSE PROJECT_SRC_RECURSE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${PROJECT_SRC_RECURSE})

file(GLOB_RECURSE CMAKE_RECURSE 
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/cmake PREFIX cmake FILES ${CMAKE_RECURSE})

# Plugin Apis for have it the visual studio tree of the App
file(GLOB_RECURSE APIS_SOURCES ${CMAKE_SOURCE_DIR}/apis/*.*)
source_group(TREE ${CMAKE_SOURCE_DIR}/apis PREFIX PluginApis FILES ${APIS_SOURCES})

set(ENABLE_EXPORTS ON CACHE STRING "" FORCE)

add_library(${PROJECT} MODULE
	${APIS_SOURCES}
	${CMAKE_RECURSE}
	${PROJECT_SRC_RECURSE}
	${PROJECT_SHADERS_RECURSE}
)

if ("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
	set(ARCH x64)
else()
	set(ARCH x32)
endif()

set_target_properties(${PROJECT} PROPERTIES	OUTPUT_NAME "${PROJECT}")
target_compile_definitions(${PROJECT} PRIVATE BUILD_CTOOLS_SHARED_LIBS)	
target_compile_definitions(${PROJECT} PRIVATE BUILD_IMGUI_PACK_SHARED_LIBS)

if(USE_DEBUG_SANITIZER)
	target_compile_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan>)
	target_link_options(${PROJECT} PRIVATE $<$<CONFIG:Debug>:-fsanitize=address -static-libasan>)
	message("Address Sanitizer enabled for projet ${PROJECT}")
endif()

set(FINAL_BIN_DIR ${CMAKE_SOURCE_DIR}/bin/plugins)
message("Plugin ${PROJECT} => ${FINAL_BIN_DIR}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}/debug")

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME_DEBUG "${PLUGIN_NAME_PREFIX}_${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}_Debug")
set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME_RELEASE "${PLUGIN_NAME_PREFIX}_${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}_Release")
set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME_MINSIZEREL "${PLUGIN_NAME_PREFIX}_${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}_MinSizeRel")
set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME_RELWITHDEBINFO "${PLUGIN_NAME_PREFIX}_${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}_RelWithDebInfo")

set_target_properties(${PROJECT} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

set_target_properties(${PROJECT} PROPERTIES FOLDER Plugins/Scripting)
			
install(IMPORTED_RUNTIME_ARTIFACTS ${PROJECT} DESTINATION plugins COMPONENT PLUGIN_BINARY_${PROJECT})

target_include_directories(${PROJECT} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}
	${IMGUIPACK_INCLUDE_DIRS}
	${EZLIBS_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT}
	${IMGUIPACK_LIBRARIES}
	libpython-static
)

target_link_directories(${PROJECT} PRIVATE
	${Python_LIBRARY_DIRS}
	${FINAL_BIN_DIR}/python
)

target_include_directories(${PROJECT} PRIVATE 
	${Python_INCLUDE_DIRS}
	${FINAL_BIN_DIR}/python/include
)

## get_target_property(PYTHON_type Python::Python TYPE)
## message("${PYTHON_type}")
## if(PYTHON_type STREQUAL "SHARED_LIBRARY")
##     add_custom_command(TARGET ${PROJECT} POST_BUILD
##                         COMMAND ${CMAKE_COMMAND} -E copy_if_different
##                                 $<TARGET_FILE:Python::Python>
##                                 $<TARGET_FILE_DIR:${PROJECT}>)
## endif()

install(FILES ${FINAL_BIN_DIR}/python/python313.dll DESTINATION plugins COMPONENT PLUGIN_LIBS_PYTHON)

# add plugin name in parent LOADED_STROCKER_PLUGINS list
set(LOADED_LOG_TO_GRAPH_PLUGINS ${LOADED_LOG_TO_GRAPH_PLUGINS} ${PROJECT} PARENT_SCOPE)

########################################################
################# BUILD INCREMENT TOOL #################
########################################################

## only for Debug
add_custom_command(
	TARGET ${PROJECT} PRE_BUILD
	COMMAND BuildInc -prefix=${PROJECT} 1000000:10 $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/src/Headers/${PROJECT}Build.h>
	DEPENDS ${PROJECT}
)

########################################################
########################################################
########################################################

endif() #PLUGIN_ENABLE_PYTHON_SCRIPTING
