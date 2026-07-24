cmake_minimum_required(VERSION 3.20)

# LuaScripting plugin test suites (googletest) — headless, no ImGui / host app involved:
#  - TestLuaRegex.cpp : the ltg:regex(...) brick (std::regex port), direct C++ calls
#  - TestModule.cpp   : the whole scripting module against a fake Ltg::IDatasModel
#                       (compile / start / parse / end flow, error routing, completion catalog)

set(TEST_PROJECT LuaScripting_Tests)

file(GLOB PLUGIN_TEST_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(Tests FILES ${PLUGIN_TEST_SRC})

add_executable(${TEST_PROJECT}
	${PLUGIN_TEST_SRC}
	${CMAKE_CURRENT_SOURCE_DIR}/../src/modules/Module.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../src/modules/LuaDatasModel.cpp
)

set_target_properties(${TEST_PROJECT} PROPERTIES FOLDER Tests)

## same sol2 config as the plugin itself — MUST reach every TU before <sol/sol.hpp>
target_compile_definitions(${TEST_PROJECT} PRIVATE SOL_ALL_SAFETIES_ON=1)
target_compile_definitions(${TEST_PROJECT} PRIVATE SOL_EXCEPTIONS_SAFE_PROPAGATION=0)

target_include_directories(${TEST_PROJECT} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty
	${CMAKE_CURRENT_SOURCE_DIR}/../src
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${LUA_JIT_INCLUDE_DIR}/src
	${EZLIBS_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}
	${SOL2_INCLUDE_DIR}
)

## gtest WITHOUT gtest_main: main.cpp installs the ez::Log singleton around RUN_ALL_TESTS
target_link_libraries(${TEST_PROJECT} PRIVATE
	${LUA_JIT_LIBRARIES}
	${SOL2_LIBRARIES}
	gtest
)

if(MSVC)
	set_property(TARGET ${TEST_PROJECT} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
endif()

gtest_discover_tests(${TEST_PROJECT})
