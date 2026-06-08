
# GLFW Options
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Detect if we're in a CI environment or if X11 is not available
if(NOT DEFINED GLFW_BUILD_X11)
    set(GLFW_BUILD_X11 ON)
endif()
if(NOT DEFINED GLFW_BUILD_WAYLAND)
    set(GLFW_BUILD_WAYLAND OFF)
endif()
set(GLFW_BUILD_WAYLAND ${GLFW_BUILD_WAYLAND} CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ${GLFW_BUILD_X11} CACHE BOOL "" FORCE)

# Download GLFW from tar.gz archive
FetchContent_Declare(glfw
    URL ${CMAKE_SOURCE_DIR}/3rdparty/libs/glfw-3.4.tar.gz
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(glfw)
set_target_properties(glfw PROPERTIES FOLDER 3rdparty)
if(TARGET update_mappings)
	set_target_properties(update_mappings PROPERTIES FOLDER 3rdparty)
endif()

# expose to the root CMakeLists (it uses these vars directly)
set(GLFW_INCLUDE_DIR ${glfw_SOURCE_DIR}/include CACHE INTERNAL "")
set(GLFW_LIBRARIES glfw CACHE INTERNAL "")

if(MSVC)
	set_property(TARGET glfw PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
