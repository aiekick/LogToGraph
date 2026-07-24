# sqlite3 (vendored in 3rdparty/sqlite3) — exposes sqlite3.hpp/sqlite3.h + sqlite3 static lib

set(SQLITE3_SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/sqlite3)

add_library(sqlite3 STATIC
    ${SQLITE3_SOURCE_DIR}/src/sqlite3.c
    ${SQLITE3_SOURCE_DIR}/include/sqlite3.h
    ${SQLITE3_SOURCE_DIR}/include/sqlite3ext.h
    ${SQLITE3_SOURCE_DIR}/sqlite3.hpp
)
target_include_directories(sqlite3 PUBLIC
    ${SQLITE3_SOURCE_DIR}
    ${SQLITE3_SOURCE_DIR}/include
)
set_target_properties(sqlite3 PROPERTIES FOLDER 3rdparty)
if (MSVC)
    target_compile_definitions(sqlite3 PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# expose to the root CMakeLists (it uses these vars directly)
set(SQLITE3_INCLUDE_DIR ${SQLITE3_SOURCE_DIR};${SQLITE3_SOURCE_DIR}/include CACHE INTERNAL "")
set(SQLITE3_LIBRARIES sqlite3 CACHE INTERNAL "")

if (MSVC)
	set_property(TARGET sqlite3 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
endif()
