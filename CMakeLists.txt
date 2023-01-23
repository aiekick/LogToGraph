cmake_minimum_required(VERSION 3.1)

set(PROJECT LogToGraph)

enable_language(C CXX)

project(${PROJECT} CXX)

set(LLVM_USE_CRT_DEBUG MTd CACHE STRING "" FORCE)
set(LLVM_USE_CRT_MINSIZEREL MT CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELEASE MT CACHE STRING "" FORCE)
set(LLVM_USE_CRT_RELWITHDEBINFO MT CACHE STRING "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL ON CACHE BOOL "")

set(USE_CONFIG_SYSTEM ON CACHE BOOL "")
set(USE_GLFW_CLIPBOARD ON CACHE BOOL "")
set(USE_GL_VERSION_CHECKER ON CACHE BOOL "")

## for group smake targets in the dir cmakeTargets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "cmakeTargets")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## some defines for debug mode (before 3rdparty.cmake)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

add_definitions(-DCUSTOM_IMGUIFILEDIALOG_CONFIG="../../../src/Gui/CustomImGuiFileDialogConfig.h")
add_definitions(-DIMGUI_USER_CONFIG="../../../src/Gui/CustomImConfig.h")
add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)
add_definitions(${GLFW_DEFINITIONS})
add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS)
add_definitions(-DIMGUI_ENABLE_FREETYPE)
add_definitions(-DUSE_IMGUI="${CMAKE_SOURCE_DIR}/3rdparty/imgui/imgui.h")
add_definitions(-DPROJECT_NAME="${PROJECT}")
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

include(cmake/3rdparty.cmake)

if(uninstall)
	set_target_properties(uninstall PROPERTIES FOLDER "cmakeTargets")
endif()

## other define
add_definitions(-DPROJECT_PATH="${CMAKE_SOURCE_DIR}")

set(FINAL_BIN_DIR ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_INSTALL_PREFIX "${FINAL_BIN_DIR}")
message("${PROJECT} => ${FINAL_BIN_DIR}")

if ("${CMAKE_EXE_LINKER_FLAGS}" STREQUAL "/machine:x64")
	set(ARCH x64)
else()
	set(ARCH x32)
endif()

if(UNIX)
    if(APPLE)
        add_definitions(-DAPPLE)
    else()
        add_definitions(-DLINUX)
    endif()
    add_definitions(-DUNIX)
elseif(WIN32)
    add_definitions(-DWIN32)
	if(MINGW)
		add_definitions(-DMINGW32)
	endif()
    set(DIRENT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/dirent/include)
    include_directories(${DIRENT_INCLUDE_DIR})
endif()

add_definitions(-DUSE_DECORATIONS_FOR_RESIZE_CHILD_WINDOWS) ## for the resize imgui issue when we have child glfw windows

if(MSVC)
    # Ignore 4055 for glad
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /IGNORE:4055 /std:c++17")
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DMSVC)
	add_definitions(-DNOMINMAX) ## for avoid issue regarding std::max and std::min
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++17 -Wdeprecated-declarations -Wunused-parameter")
endif ()

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX _debug)
endif()

file(GLOB PROJECT_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
source_group(main FILES ${PROJECT_MAIN})

file(GLOB_RECURSE SRC_CMAKE
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/cmake PREFIX cmake FILES ${SRC_CMAKE})

file(GLOB_RECURSE SRC_LUA
	${CMAKE_CURRENT_SOURCE_DIR}/doc/*.lua)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/doc PREFIX lua_samples FILES ${SRC_LUA})

file(GLOB_RECURSE PROJECT_SRC_RECURSE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${PROJECT_SRC_RECURSE})

file(GLOB IMGUIIMPL_SOURCES 
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_glfw.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_glfw.h
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_opengl3.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/backends/imgui_impl_opengl3.h)
source_group(src\\ImGui_Impl FILES ${IMGUIIMPL_SOURCES})

file(GLOB IMGUI_FREETYPE_SOURCES
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/misc/freetype/imgui_freetype.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/imgui/misc/freetype/imgui_freetype.h)
source_group(src\\ImGui_FreeType FILES ${IMGUI_FREETYPE_SOURCES})

if (MSVC)
	set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/res/appicon.rc")
endif()

add_executable(${PROJECT}
	${SRC_LUA}
	${SRC_CMAKE}
	${PROJECT_MAIN}
	${IMGUIIMPL_SOURCES}
	${PROJECT_SRC_RECURSE}
	${IMGUI_FREETYPE_SOURCES}
	${APP_ICON_RESOURCE_WINDOWS})

include_directories(
	src
	3rdparty
##	${LUA_INCLUDE_DIR}
	${LUA_JIT_INCLUDE_DIR}
	${PLOG_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${GLAD_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
	${IMPLOT_INCLUDE_DIR}
	${TINYXML2_INCLUDE_DIR}
	${FREETYPE_INCLUDE_DIR}
	${IMGUIFILEDIALOG_INCLUDE_DIR}
	${IMGUICOLORTEXTEDIT_INCLUDE_DIR}
)

target_link_libraries(${PROJECT} PRIVATE
##	${LUA_LIBRARIES}
	${LUA_JIT_LIBRARIES}
	${GLFW_LIBRARIES}
	${GLAD_LIBRARIES}
	${IMGUI_LIBRARIES}
	${OPENGL_LIBRARIES}
	${IMPLOT_LIBRARIES}
	${CTOOLS_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${TINYXML2_LIBRARIES}
	${IMGUIFILEDIALOG_LIBRARIES}
	${IMGUICOLORTEXTEDIT_LIBRARIES}
)

set(${PROJECT}_Output_Name "${PROJECT}_${CMAKE_SYSTEM_NAME}$<$<CONFIG:Debug>:_Debug>$<$<CONFIG:Release>:_Release>$<$<CONFIG:MinSizeRel>:_MinSizeRel>$<$<CONFIG:RelWithDebInfo>:_RelWithDebInfo>_${ARCH}")
set_target_properties(${PROJECT} PROPERTIES	OUTPUT_NAME "${${PROJECT}_Output_Name}")

set_target_properties(${PROJECT} PROPERTIES	RUNTIME_OUTPUT_DIRECTORY "${FINAL_BIN_DIR}")

########################################################
################# BUILD INCREMENT TOOL #################
########################################################

## only for Debug
add_subdirectory(3rdparty/buildInc)
set_target_properties(BuildInc PROPERTIES FOLDER 3rdparty/tools)

add_custom_command(
	TARGET ${PROJECT} PRE_BUILD
	COMMAND BuildInc -prefix=${PROJECT} 1000:10 $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/src/Headers/${PROJECT}Build.h>
	DEPENDS ${PROJECT}
)

########################################################
################# Dependencies copy in bin dir #########
########################################################

if(MSVC)
	## only for Release
	## add_custom_command(
	## 	TARGET ${PROJECT} POST_BUILD
	## 	COMMAND $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/3rdparty/pedeps/bin/copypedeps.exe> -r $<$<CONFIG:Release>:${FINAL_BIN_DIR}/Release/${${PROJECT}_Output_Name}.exe> ${FINAL_BIN_DIR}/Release
	## 	DEPENDS ${PROJECT}
	## )
endif()

#############################################################
### OUTPUT NAME #############################################
#############################################################

if(WIN32)
	if (MINGW)
		set(${PROJECT}_Output_Name "${PROJECT}_Mingw32_${ARCH}")
	elseif (CYGWIN)
		set(${PROJECT}_Output_Name "${PROJECT}_Cygwin_${ARCH}")
	elseif(MSVC)
		set(${PROJECT}_Output_Name "${PROJECT}_Msvc_${ARCH}")
	endif()
	set_target_properties(${PROJECT} PROPERTIES	OUTPUT_NAME "${${PROJECT}_Output_Name}")
elseif(UNIX)
	set(${PROJECT}_Output_Name "${PROJECT}_Linux_${ARCH}")
	set_target_properties(${PROJECT} PROPERTIES	OUTPUT_NAME "${${PROJECT}_Output_Name}")
endif()

#############################################################
### INSTALL #################################################
#############################################################

install(TARGETS ${PROJECT} RUNTIME DESTINATION / COMPONENT APP_BINARY)

if (WIN32)
install(FILES 
	C:/Windows/System32/downlevel/api-ms-win-crt-convert-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-environment-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-filesystem-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-heap-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-locale-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-math-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-runtime-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-stdio-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-string-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-time-l1-1-0.dll
	C:/Windows/System32/downlevel/api-ms-win-crt-utility-l1-1-0.dll
	C:/Windows/System32/MSVCP140.dll
	C:/Windows/System32/VCRUNTIME140.dll
	C:/Windows/System32/VCRUNTIME140_1.dll
    DESTINATION /
	COMPONENT APP_FILES)
endif()

#############################################################
### VERSION #################################################
#############################################################

file(READ ${CMAKE_SOURCE_DIR}/src/Headers/LogToGraphBuild.h BuildIdFile)
string(REGEX MATCH "#define ${PROJECT}_BuildNumber ([0-9]*)" _ ${BuildIdFile})
set(BuildNumber ${CMAKE_MATCH_1})
string(REGEX MATCH "#define ${PROJECT}_MinorNumber ([0-9]*)" _ ${BuildIdFile})
set(MinorNumber ${CMAKE_MATCH_1})
string(REGEX MATCH "#define ${PROJECT}_MajorNumber ([0-9]*)" _ ${BuildIdFile})
set(MajorNumber ${CMAKE_MATCH_1})
set(${PROJECT}_Version "v${MajorNumber}.${MinorNumber}.${BuildNumber}")
message("${PROJECT} Version to install : ${${PROJECT}_Version}")

#############################################################
### PACK ####################################################
## https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
#############################################################

if (USE_PACKAGE_TO_NSIS)
	set(CPACK_GENERATOR "NSIS")
else()
	set(CPACK_GENERATOR "ZIP")
endif()
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_ZIP_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)
set(CPACK_PACKAGE_FILE_NAME "${${PROJECT}_Output_Name}_v${MajorNumber}.${MinorNumber}.${BuildNumber}")
set(CPACK_PACKAGE_VENDOR "FunParadigm")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${${PROJECT}_Output_Name} - log analyzer with lua")
set(CPACK_PACKAGE_VERSION "${MajorNumber}.${MinorNumber}.${BuildNumber}")
set(CPACK_PACKAGE_VERSION_MAJOR "${MajorNumber}")
set(CPACK_PACKAGE_VERSION_MINOR "${MinorNumber}")
set(CPACK_PACKAGE_VERSION_PATCH "${BuildNumber}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${FINAL_BIN_DIR}")
set(CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "${CMAKE_PROJECT_NAME}")
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_INSTALL_PREFIX}")
include(CPack)
