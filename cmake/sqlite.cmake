set(SQLITE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/sqlite)
                 
add_library(sqlite STATIC 
	${SQLITE_INCLUDE_DIR}/sqlite3.c
	${SQLITE_INCLUDE_DIR}/sqlite3.h
	${SQLITE_INCLUDE_DIR}/sqlite3ext.h)
    
set_target_properties(sqlite PROPERTIES LINKER_LANGUAGE C)
set_target_properties(sqlite PROPERTIES FOLDER 3rdparty)

set(SQLITE_LIBRARIES sqlite)
