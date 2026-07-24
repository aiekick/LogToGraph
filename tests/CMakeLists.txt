cmake_minimum_required(VERSION 3.20)

# App test suites (googletest) — headless units behind the code editor:
#  - TestCodeUtils.cpp      : pure text-analysis helpers (token / call-target / signature scan / filter)
#  - TestImCodeContract.cpp : the im::Code model behaviors the CodeEditor port relies on
#                             (byte columns, selection-replace, search, undo, lexer names)

set(TEST_PROJECT LogToGraph_Tests)

file(GLOB PROJECT_TEST_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(Tests FILES ${PROJECT_TEST_SRC})

add_executable(${TEST_PROJECT}
	${PROJECT_TEST_SRC}
	${CMAKE_SOURCE_DIR}/src/frontend/Components/CodeUtils.cpp
)

set_target_properties(${TEST_PROJECT} PROPERTIES FOLDER Tests)

target_include_directories(${TEST_PROJECT} PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/3rdparty
	${IMGUIPACK_INCLUDE_DIRS}
	${EZLIBS_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
	## imguipack.h pulls the embedded profiler config (CUSTOM_IN_APP_GPU_PROFILER_CONFIG),
	## which includes glad + glfw headers
	${GLAD_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
)

target_link_libraries(${TEST_PROJECT} PRIVATE
	imguipack
	gtest_main
)

if(MSVC)
	set_property(TARGET ${TEST_PROJECT} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
endif()

gtest_discover_tests(${TEST_PROJECT})
