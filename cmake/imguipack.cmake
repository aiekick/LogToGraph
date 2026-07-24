set(IMGUIPACK_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/imguipack)
set(IMGUIPACK_LIBRARIES imguipack)

# ON
set(USE_IM_TOOLS ON CACHE BOOL "" FORCE)
set(USE_IM_GENIE ON CACHE BOOL "" FORCE)
set(USE_IM_NODAL ON CACHE BOOL "" FORCE)
set(USE_IM_LAYOUT ON CACHE BOOL "" FORCE)
set(USE_IM_COOL_BAR ON CACHE BOOL "" FORCE)
set(USE_IMGUI_FILE_DIALOG ON CACHE BOOL "" FORCE)
set(USE_IMGUI_COLOR_TEXT_EDIT ON CACHE BOOL "" FORCE)
set(USE_EMBEDDED_FRAME_PROFILER ON CACHE BOOL "" FORCE) ## ProfilerPane / IAGP* macros need iagp
set(IMGUIPACK_USE_STD_FILESYSTEM ON CACHE BOOL "" FORCE)

# OFF
set(USE_IMPLOT ON CACHE BOOL "" FORCE) ## graph panes need ImPlot
set(USE_IM_GUIZMO OFF CACHE BOOL "" FORCE)
set(USE_IMGUI_MARKDOW OFF CACHE BOOL "" FORCE)
set(USE_IM_GRADIENT_HDR OFF CACHE BOOL "" FORCE)

add_subdirectory(${IMGUIPACK_SOURCE_DIR} EXCLUDE_FROM_ALL)

set_target_properties(imguipack PROPERTIES FOLDER 3rdparty)
set_target_properties(freetype PROPERTIES FOLDER 3rdparty)
if(TARGET boost_regex)
	set_target_properties(boost_regex PROPERTIES FOLDER 3rdparty)
endif()

set_target_properties(imguipack PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(imguipack PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(imguipack PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(imguipack PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

target_include_directories(imguipack PUBLIC
	${CMAKE_SOURCE_DIR}/3rdparty/ezlibs/include
)

## CustomInAppGpuProfiler.h (consumed via add_definitions) pulls glad+glfw — wire them
## into imguipack itself so the embedded frame profiler TU compiles
target_include_directories(imguipack PRIVATE
	${GLAD_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
)

set_target_properties(freetype PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(freetype PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(freetype PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(freetype PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

if(TARGET boost_regex)
	set_target_properties(boost_regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
	set_target_properties(boost_regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
	set_target_properties(boost_regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
	set_target_properties(boost_regex PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")
endif()

if (MSVC)
	set_property(TARGET imguipack PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
	set_property(TARGET freetype PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
	if(TARGET boost_regex)
		set_property(TARGET boost_regex PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>") # on garde DLL only in debug mode, since MSVC bad debugging without
	endif()
endif()

if(TARGET glfw)
	target_link_libraries(imguipack
		glfw
	)
endif()
