# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)

# 项目信息
project (my_bus_mgr_lib_prj CXX)

INCLUDE_DIRECTORIES(system ../external/include/)
INCLUDE_DIRECTORIES(system ../lib_prj/)

# 查找当前目录下的所有源文件
# 并将名称保存到 ALL_SRC_FILE 变量
file(GLOB_RECURSE ALL_SRC_FILE  ${PROJECT_SOURCE_DIR} "*.cc")
file(GLOB_RECURSE ALL_SRC_FILE2  ${PROJECT_SOURCE_DIR} "*.cpp")


# 指定生成目标
ADD_LIBRARY(bus_mgr_lib STATIC ${ALL_SRC_FILE} ${ALL_SRC_FILE2} )
#cotire(base)

#设置依赖
ADD_DEPENDENCIES(bus_mgr_lib lib_prj)
# 设置链接库
target_link_libraries(bus_mgr_lib easy_libevent_lib)