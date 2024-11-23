FetchContent_Declare(
	alphanum
	GIT_REPOSITORY	https://github.com/aiekick/alphanum.git
	GIT_TAG			1f9a72cc50543fc2b4a9e2aba8ab5b5fbb103f54
	SOURCE_DIR		${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/alphanum
	GIT_PROGRESS	true
	GIT_SHALLOW		true
)

FetchContent_GetProperties(alphanum)
if(NOT alphanum_POPULATED)
	FetchContent_Populate(alphanum)
	
	set(ALPHANUM_INCLUDE_DIR ${alphanum_SOURCE_DIR})
endif()