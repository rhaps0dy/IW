cmake_minimum_required(VERSION 2.8)

project(IteratedWidth)

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11") 
add_definitions(-DROM_DIR="${ROM_DIR}" -DTFG_DIR="${TFG_DIR}")

set(ALE_DIR "/home/adria/Programacio/Arcade-Learning-Environment" CACHE FILEPATH "Repository root for the Arcade Learning Environment")

include_directories(${ALE_DIR}/src)
link_directories(${ALE_DIR})


set(OTHERS IW1Search.cpp SearchTree.cpp TreeNode.cpp bit_array.cxx)

add_executable(iw main.cpp ${OTHERS})
target_link_libraries(iw ale)

add_executable(replay replay.cpp)
target_link_libraries(replay ale)

if(LINUX)
set_target_properties(iw PROPERTIES LINK_FLAGS -Wl,-rpath,${ALE_DIR})
set_target_properties(replay PROPERTIES LINK_FLAGS -Wl,-rpath,${ALE_DIR})
endif(LINUX)



install(TARGETS iw DESTINATION .)
