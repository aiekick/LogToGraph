cmake_minimum_required(VERSION 3.20)

set(PROJECT LogToGraph)

enable_language(C CXX)
project(${PROJECT} CXX)

option(USE_BUILDING_AS_SHARED "Enable shared building" ON)
option(USE_BUILDING_OF_TESTS "Enable the build of tests" OFF)

## for group smake targets in the dir CmakeTargets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CmakeTargets")

## show the dependency graph of cmake
## set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## some defines for debug mode (before 3rdparty.cmake)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

set(FINAL_BIN_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}/debug")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}/release")
set(CMAKE_PDB_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}/minsizerel")
set(CMAKE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}/reldeb")

set(CMAKE_INSTALL_PREFIX .)
set(CMAKE_INSTALL_BINDIR .)

set(USE_SHARED_LIBS ${USE_BUILDING_AS_SHARED} CACHE BOOL "" FORCE)

if ("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
	set(ARCH x64)
else()
	set(ARCH x32)
endif()

# some times for unknown reason, the lib file is not generated so no symbols are exported.
# this option force all sahred lib to export theirs symbols
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ${USE_SHARED_LIBS} CACHE BOOL "" FORCE)

if ("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
	set(ARCH x64)
else()
	set(ARCH x32)
endif()

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX _debug)
endif()
if(NOT CMAKE_RELEASE_POSTFIX)
  set(CMAKE_RELEASE_POSTFIX) ## to config in file
endif()
if(NOT CMAKE_MINSIZEREL_POSTFIX)
  set(CMAKE_MINSIZEREL_POSTFIX _minsizerel)
endif()
if(NOT CMAKE_RELWITHDEBINFO_POSTFIX)
  set(CMAKE_RELWITHDEBINFO_POSTFIX _reldeb)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zo")
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DMSVC)
	add_definitions(-DNOMINMAX)
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wunused-parameter -Wno-unknown-pragmas")
endif ()

if(WIN32)
	add_definitions(-DWIN32)
endif()

add_definitions(-DUSE_XML_CONFIG)
add_definitions(-DUSE_STD_FILESYSTEM)
add_definitions(-DIMGUI_ENABLE_FREETYPE)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)
add_definitions(-DUSE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS)
add_definitions(-DILAYOUT_PANE_INCLUDE="${CMAKE_SOURCE_DIR}/apis/ILayoutPane.h")
add_definitions(-DIMGUI_INCLUDE="${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/ImGuiPack.h")
add_definitions(-DIMGUI_USER_CONFIG="${CMAKE_SOURCE_DIR}/src/headers/CustomImGuiConfig.h")
add_definitions(-DCUSTOM_IMWIDGETS_CONFIG="${CMAKE_SOURCE_DIR}/src/headers/CustomImWidgetsConfig.h")
add_definitions(-DCUSTOM_IN_APP_GPU_PROFILER_CONFIG="${CMAKE_SOURCE_DIR}/src/headers/CustomInAppGpuProfiler.h")
add_definitions(-DCUSTOM_IMGUIFILEDIALOG_CONFIG="${CMAKE_SOURCE_DIR}/src/headers/CustomImGuiFileDialogConfig.h")
add_definitions(-DCUSTOM_STROCKER_BACKEND_CONFIG="${CMAKE_SOURCE_DIR}/src/headers/CustomStrockerBackendConfig.h")

include(cmake/3rdparty.cmake)

###########################################################
##### PLUGINS PREFIX ######################################
###########################################################

### each plugins must start by this

set(PLUGIN_NAME_PREFIX "Plugin")
add_definitions(-DPLUGIN_NAME_PREFIX="${PLUGIN_NAME_PREFIX}")

###########################################################
##### SEARCH FOR PLUGINS ##################################
###########################################################

set(LOADED_STROCKER_PLUGINS) ### musty be fillerd by each pluigns in CMakeLists.txt with plugin name
set(PLUGINS_DIRECTORY ${CMAKE_SOURCE_DIR}/plugins)
file(GLOB SUBDIRS RELATIVE ${PLUGINS_DIRECTORY} ${PLUGINS_DIRECTORY}/*)
FOREACH(SUBDIR ${SUBDIRS})
	if(IS_DIRECTORY ${PLUGINS_DIRECTORY}/${SUBDIR})
		if (EXISTS ${PLUGINS_DIRECTORY}/${SUBDIR}/CMakeLists.txt)
			add_subdirectory(${PLUGINS_DIRECTORY}/${SUBDIR})
		endif()
	endif()
ENDFOREACH()

###########################################################
###########################################################
###########################################################

add_definitions(${GLFW_DEFINITIONS})
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Plugin Apis for have it the visual studio tree of the App
file(GLOB_RECURSE APIS_SOURCES ${CMAKE_SOURCE_DIR}/apis/*.*)
source_group(TREE ${CMAKE_SOURCE_DIR}/apis PREFIX PluginApis FILES ${APIS_SOURCES})

# Main sources
file(GLOB MAIN_SOURCES ${CMAKE_SOURCE_DIR}/main.cpp)
source_group(main FILES ${MAIN_SOURCES})

# App Sources
file(GLOB_RECURSE SRC_SOURCES ${CMAKE_SOURCE_DIR}/src/*.*)
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX src FILES ${SRC_SOURCES})

# cmake Sources for have it the visual studio tree of the App
file(GLOB_RECURSE PROJECT_CMAKE_FILES
		${CMAKE_SOURCE_DIR}/cmake/*.cmake)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/cmake PREFIX cmake FILES ${PROJECT_CMAKE_FILES})

file(GLOB IMGUI_IMPL_SOURCES 
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/backends/imgui_impl_glfw.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/backends/imgui_impl_glfw.h
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/backends/imgui_impl_opengl3.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/backends/imgui_impl_opengl3.h
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/misc/freetype/imgui_freetype.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/ImGuiPack/3rdparty/imgui_docking/misc/freetype/imgui_freetype.h)
source_group(src\\ImGuiImpl FILES ${IMGUI_IMPL_SOURCES})

add_executable(${PROJECT}
	${SRC_SOURCES}
	${MAIN_SOURCES}
	${APIS_SOURCES}
	${EZ_LIBS_SOURCES}
	${IMGUI_IMPL_SOURCES}
	${PROJECT_CMAKE_FILES}
)

if (USE_SHARED_LIBS)
	target_compile_definitions(${PROJECT} PRIVATE BUILD_IMGUI_PACK_SHARED_LIBS)	
	target_compile_definitions(${PROJECT} PRIVATE BUILD_IN_APP_GPU_PROFILER_SHARED_LIBS)
	target_compile_definitions(${PROJECT} PRIVATE BUILD_STROCKER_BRIDGE_SHARED_LIBS)
endif()

set_target_properties(${PROJECT} PROPERTIES OUTPUT_NAME "${PROJECT}_${CMAKE_SYSTEM_NAME}_${ARCH}")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")
install(TARGETS ${PROJECT} DESTINATION ${CMAKE_INSTALL_PREFIX})

include_directories(
	${STROCKER_BRIDGE_INCLUDE_DIRS}
	${IN_APP_GPU_PROFILER_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/3rdparty
	${IMGUIPACK_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/src
	${TINYXML2_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${CTOOLS_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
	${PLOG_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
	${CURL_INCLUDE_DIR}
	${FMT_INCLUDE_DIR}
)

target_link_libraries(${PROJECT}
	${STROCKER_BRIDGE_LIBRARIES}
	${IN_APP_GPU_PROFILER_LIBRARIES}
	${IMGUIPACK_LIBRARIES}
	${TINYXML2_LIBRARIES}
	${SQLITE3_LIBRARIES}
	${OPENGL_LIBRARIES}
	${CTOOLS_LIBRARIES}
	${GLFW_LIBRARIES}
	${GLAD_LIBRARIES}
	${CURL_LIBRARIES}
	${FMT_LIBRARIES}
)

# define for let Strocker detext what is the config of a plugin 
target_compile_definitions(${PROJECT} PRIVATE -DPLUGIN_RUNTIME_CONFIG=$<$<CONFIG:Debug>:"_Debug">$<$<CONFIG:Release>:"_Release">$<$<CONFIG:MinSizeRel>:"_MinSizeRel">$<$<CONFIG:RelWithDebInfo>:"_RelWithDebInfo">)
target_compile_definitions(${PROJECT} PRIVATE -DPROJECT_EXT=".stro")
target_compile_definitions(${PROJECT} PRIVATE -DPROJECT_EXT_DOT_LESS="stro")

set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(${PROJECT} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

########################################################
############## FORCE BUILD OF PLUGINS ##################
########################################################

FOREACH(PLUGIN ${LOADED_STROCKER_PLUGINS})
	add_dependencies(${PROJECT} ${PLUGIN})
ENDFOREACH()

########################################################
################# BUILD INCREMENT TOOL #################
########################################################

## only for Debug
add_subdirectory(3rdparty/BuildInc)
## set_target_properties(BuildInc PROPERTIES FOLDER Tools)

add_custom_command(
	TARGET ${PROJECT} PRE_BUILD
	COMMAND BuildInc -prefix=${PROJECT} 1000000:10 $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/src/Headers/${PROJECT}Build.h>
	DEPENDS ${PROJECT})

########################################################
##### TESTS ############################################
########################################################

if(USE_BUILDING_OF_TESTS)
	enable_testing()
endif()
