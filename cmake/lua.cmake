set(LUA_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/lua)

file(GLOB LUA_SOURCES 
	${LUA_INCLUDE_DIR}/src/*.c)
file(GLOB LUA_HEADERS 
	${LUA_INCLUDE_DIR}/src/*.h 
	${LUA_INCLUDE_DIR}/src/*.hpp)
                 
add_library(lua STATIC ${LUA_SOURCES} ${LUA_HEADERS})

include_directories(
    ${LUA_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR})
    
set_target_properties(lua PROPERTIES LINKER_LANGUAGE C)
set_target_properties(lua PROPERTIES FOLDER 3rdparty)

set(LUA_LIBRARIES lua)

