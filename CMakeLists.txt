cmake_minimum_required(VERSION 2.8)

project(IteratedWidth)

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(PRINT OFF CACHE BOOL "Whether to print exploration trace")
if(PRINT)
	add_definitions(-DPRINT)
endif(PRINT)
set(OUTPUT_EXPLORE OFF CACHE BOOL "Whether to output exploration information")
if(OUTPUT_EXPLORE)
	add_definitions(-DOUTPUT_EXPLORE)
endif(OUTPUT_EXPLORE)
set(SHOW_SEARCH OFF CACHE BOOL "Whether to display the search process on the screen")
if(SHOW_SEARCH)
	add_definitions(-DSHOW_SEARCH)
endif(SHOW_SEARCH)


set(ALE_DIR "../ALE-montezuma-modified" CACHE FILEPATH "Repository root for the Arcade Learning Environment")

include_directories(${ALE_DIR}/src)
link_directories(${ALE_DIR})


set(OTHERS IW1Search.cpp SearchTree.cpp TreeNode.cpp bit_array.cxx IW3OnlySearch.cpp AbstractIWSearch.cpp)

add_executable(iw main.cpp ${OTHERS})
target_link_libraries(iw ale)

add_executable(replay replay.cpp)
target_link_libraries(replay ale)

if(LINUX)
set_target_properties(iw PROPERTIES LINK_FLAGS -Wl,-rpath,${ALE_DIR})
set_target_properties(replay PROPERTIES LINK_FLAGS -Wl,-rpath,${ALE_DIR})
endif(LINUX)



install(TARGETS iw DESTINATION .)
