set(EZLIBS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/ezlibs/include)

set(USE_EZ_LIBS_TESTING ${USE_BUILDING_OF_TESTS} CACHE BOOL "" FORCE)

set(TESTING_APP ON CACHE BOOL "" FORCE)
set(TESTING_FILE ON CACHE BOOL "" FORCE)
set(TESTING_MATH ON CACHE BOOL "" FORCE)
set(TESTING_MISC ON CACHE BOOL "" FORCE)
set(TESTING_TIME ON CACHE BOOL "" FORCE)
## OFF: the ezlibs GL suites need a live OpenGL context — they can only run on a machine
## with a GPU/display, not on a headless test runner
set(TESTING_OPENGL OFF CACHE BOOL "" FORCE)
set(TESTING_COMPRESSION OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ezlibs)
