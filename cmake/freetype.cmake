set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(FT_WITH_BZIP2 OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG OFF CACHE BOOL "" FORCE)
set(FT_WITH_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(FT_WITH_BROTLI OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/freetype2)

set_target_properties(freetype PROPERTIES FOLDER 3rdparty/imgui_related)

set(FREETYPE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/freetype2/include)
set(FREETYPE_LIBRARIES freetype)

add_definitions(-DFT_DEBUG_LEVEL_TRACE)