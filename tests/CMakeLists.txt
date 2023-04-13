

#this CMakeLists was created with EasyCmake - V2 
#the repository can be found at https://github.com/knz13/EasyCmake_Cpp


cmake_minimum_required(VERSION 3.20)



#adding useful functions

function(DIR_EXISTS variable dir_path)

file(GLOB ${variable}_check ${dir_path}/*)

list(LENGTH ${variable}_check ${variable}_len)

if(${${variable}_len} EQUAL 0)

set(${variable} FALSE PARENT_SCOPE)

else()

set(${variable} TRUE PARENT_SCOPE)

endif()

endfunction()

#adding extra cmake libs
include(GNUInstallDirs)
include(ExternalProject)
include(FetchContent)


#project name
project("yael_tests_proj")

# --------------------- Repository declarations ------------------------


#repository download and settings for alias catch2...


	FetchContent_Declare(catch2_project

		GIT_REPOSITORY https://github.com/catchorg/Catch2

		GIT_TAG v3.0.1

		SOURCE_DIR ${PROJECT_SOURCE_DIR}/vendor/catch2

	)

	list(APPEND ${PROJECT_NAME}_DEPENDENCIES catch2_project)


	FetchContent_MakeAvailable(${${PROJECT_NAME}_DEPENDENCIES})

	foreach(X IN LISTS ${PROJECT_NAME}_DEPENDENCIES)

		FetchContent_GetProperties(${X}
			SOURCE_DIR ${X}_SOURCE_DIR
			BINARY_DIR ${X}_BINARY_DIR
		)

	endforeach() 

#creating executable
add_executable(yael_tests


	${PROJECT_SOURCE_DIR}/tests.cpp

)

set_property(TARGET yael_tests PROPERTY CXX_STANDARD 17)


#adding libraries...


target_link_libraries(yael_tests PRIVATE Catch2::Catch2)

target_link_libraries(yael_tests PRIVATE Catch2::Catch2WithMain)


#adding includes...


target_include_directories(yael_tests PRIVATE ${PROJECT_SOURCE_DIR}/vendor/catch2/src)
