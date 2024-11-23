set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# we will fetch the library
# we are not maintening
include(cmake/NotMaintained/notmaintained.cmake)

## libe we are maintening
## order is important here
include(cmake/Maintained/maintained.cmake)
