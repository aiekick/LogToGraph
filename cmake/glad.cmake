
# Download GLFW from tar.gz archive
FetchContent_Declare(glad
    URL ${CMAKE_SOURCE_DIR}/libs/glad.tar.gz
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(glad)

# Build ImGui with integrated OpenGL loader (no GLAD needed)
add_library(glad STATIC
    ${glad_SOURCE_DIR}/src/glad.c
    ${glad_SOURCE_DIR}/include/glad/glad.h
)
target_include_directories(glad PUBLIC
    ${glad_SOURCE_DIR}/include
)

set_target_properties(glad PROPERTIES FOLDER 3rdparty)
