
# GoogleTest 1.17.0 — vendored as a tar.gz in 3rdparty/libs (same pattern as glfw/glad).
# Only pulled when USE_BUILDING_OF_TESTS is ON (see the root CMakeLists).

include(FetchContent)

# match the app's runtime selection (CMP0091 NEW + CMAKE_MSVC_RUNTIME_LIBRARY drive it;
# gtest_force_shared_crt covers googletest's legacy internal /MT override)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE) ## hand-rolled fakes are enough for now

FetchContent_Declare(googletest
    URL ${CMAKE_SOURCE_DIR}/3rdparty/libs/googletest-1.17.0.tar.gz
	DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)

foreach(GTEST_TARGET gtest gtest_main)
	if(TARGET ${GTEST_TARGET})
		set_target_properties(${GTEST_TARGET} PROPERTIES FOLDER 3rdparty)
		if(MSVC)
			set_property(TARGET ${GTEST_TARGET} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
		endif()
	endif()
endforeach()

include(GoogleTest) ## gtest_discover_tests()
