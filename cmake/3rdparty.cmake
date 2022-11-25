if (CMAKE_SYSTEM_NAME STREQUAL Linux)
  find_package(X11 REQUIRED)
  if (NOT X11_Xi_FOUND)
    message(FATAL_ERROR "X11 Xi library is required")
  endif ()
endif ()

## contrib
include(cmake/glad.cmake)
include(cmake/glfw.cmake)
include(cmake/stb.cmake)
include(cmake/implot.cmake)
include(cmake/imgui.cmake)
include(cmake/tinyxml2.cmake)
include(cmake/freetype.cmake)
include(cmake/lua.cmake)

## aiekick
include(cmake/ctools.cmake)
include(cmake/imguifiledialog.cmake)


