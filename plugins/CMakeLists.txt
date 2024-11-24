cmake_minimum_required(VERSION 3.20)

###########################################################
##### SEARCH FOR PLUGINS ##################################
###########################################################

set(PLUGINS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SUBDIRS RELATIVE ${PLUGINS_DIRECTORY} ${PLUGINS_DIRECTORY}/*)
FOREACH(SUBDIR ${SUBDIRS})
	if(IS_DIRECTORY ${PLUGINS_DIRECTORY}/${SUBDIR})
		if (EXISTS ${PLUGINS_DIRECTORY}/${SUBDIR}/CMakeLists.txt)
			add_subdirectory(${PLUGINS_DIRECTORY}/${SUBDIR})
		endif()
	endif()
ENDFOREACH()

set(LOADED_STROCKER_PLUGINS ${LOADED_STROCKER_PLUGINS} PARENT_SCOPE)

