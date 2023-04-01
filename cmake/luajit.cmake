set(LUA_JIT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/luajit)

include_directories(
    ${LUA_JIT_INCLUDE_DIR}/src	
)

# set(CMAKE_CROSSCOMPILING ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/luajit EXCLUDE_FROM_ALL)
    
set_target_properties(libluajit PROPERTIES LINKER_LANGUAGE C)
set_target_properties(luajit PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(minilua PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(libluajit PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(buildvm PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(buildvm_arch_h PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(lj_gen_folddef PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(lj_gen_headers PROPERTIES FOLDER 3rdparty/luajit)
set_target_properties(lj_gen_vm_s PROPERTIES FOLDER 3rdparty/luajit)

set(LUA_JIT_LIBRARIES libluajit)
