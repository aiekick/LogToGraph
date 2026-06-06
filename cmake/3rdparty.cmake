set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Use FetchContent for dependencies
include(FetchContent)

include(cmake/glad.cmake)
include(cmake/glfw.cmake)
include(cmake/sqlite3.cmake)
include(cmake/ezlibs.cmake)
include(cmake/imguipack.cmake)
