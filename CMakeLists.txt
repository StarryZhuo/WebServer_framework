cmake_minimum_required(VERSION 3.22.2)
project(MyServer)

# 修改显示输出的路径
# include (cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAEK_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function ")
# -Wno-builtin-macro-redefined

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)
message("***", ${YAMLCPP})

set(LIB_SRC
    MyServer/log.cc
    MyServer/util.cc
    MyServer/config.cc
    MyServer/thread.cc
   )
add_library(MyServer SHARED ${LIB_SRC})
# force_redefine_file_macro_for_sources(MyServer) #__File__
#add_library(MyServer_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (MyServer_static PROPERTIES OUTPUT_NAME "MyServer")

set(LIBS 
        MyServer
        pthread
        yaml-cpp)       


message("***", ${LIBS})

add_executable(test tests/test.cc)
add_dependencies(test MyServer)
# force_redefine_file_macro_for_sources(test) #__File__
target_link_libraries(test ${LIBS})

#表示新加的一个测试
add_executable(test_config tests/test_config.cc)
add_dependencies(test_config MyServer)
# force_redefine_file_macro_for_sources(test_config) #__File__
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread MyServer)
# force_redefine_file_macro_for_sources(test_config) #__File__
target_link_libraries(test_thread ${LIBS})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
