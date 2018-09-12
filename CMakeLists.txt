#环境变量说明:
#CMAKE_SOURCE_DIR  CMakeList.txt所在的目录
#CMAKE_SOURCE_DIR,PROJECT_SOURCE_DIR,_SOURCE_DIR： 这三个变量内容一致，都指的是工程的顶级目录。
#PROJECT_BINARY_DIR 指执行 cmake的路径

PROJECT(my_prj_name)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#cotire设置gcc的预编译头相关
SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
INCLUDE(cotire)

# include directories
INCLUDE_DIRECTORIES(system ./external/include/)
INCLUDE_DIRECTORIES(system ./libevent-2.1.8-stable/out/include)
INCLUDE_DIRECTORIES(system ./lib_prj/)
INCLUDE_DIRECTORIES(after  ${PROJECT_SOURCE_DIR})

#运行cmake输出终端消息，调试可以用
message(PROJECT_SOURCE_DIR_is: ${PROJECT_SOURCE_DIR})

# lib directories
LINK_DIRECTORIES(
  #项目生成的静态链接库
  ${PROJECT_BINARY_DIR}/lib

  #项目引用第三方库
 # ${PROJECT_SOURCE_DIR}/external/lib
  ${PROJECT_SOURCE_DIR}/libevent-2.1.8-stable/out/lib

  #系统目录
  /usr/local/lib
  /use/lib
)


#指定编译器
if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -lrt -pthread -DDEBUG -Werror -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -lrt -pthread   -Werror -DDEBUG -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
else( CMAKE_BUILD_TYPE STREQUAL "Release" )
    #set(CMAKE_BUILD_TYPE "Release")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -lrt -DGMPERMISSION -pthread -Werror -Wno-unused-result -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -lrt -O2 -DGMPERMISSION -pthread -Werror -Wno-unused-result -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
endif( CMAKE_BUILD_TYPE STREQUAL "Debug" )

#set(CMAKE_C_COMPILER /usr/bin/gcc)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
#SET(CMAKE_CPP_FLAGS_DEBUG "-g -Wall -std=c++11 -Wno-format") 

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_SUBDIRECTORY(lib_prj)
ADD_SUBDIRECTORY(server_prj)
ADD_SUBDIRECTORY(client_prj)
ADD_SUBDIRECTORY(test_bus_mgr_prj)
ADD_SUBDIRECTORY(bus_mgr_lib_prj)

#自定义命令
##################################
#ADD_CUSTOM_COMMAND(
#     OUTPUT f1
#     #DEPENDS f2 f3 f4*
#	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#     COMMAND sh build_svn_version.sh
#     COMMENT "out put build_svn_version"
#           )
#
#ADD_CUSTOM_TARGET(t2 ALL DEPENDS f1 o1)
#ADD_DEPENDENCIES(server t2)

add_custom_command(
     OUTPUT o1
	 WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
     DEPENDS  ${PROJECT_BINARY_DIR}/../f2  ${PROJECT_BINARY_DIR}/../f4*  #文件的相对路径是 ${PROJECT_SOURCE_DIR}，
     COMMAND cat ../f2 > o1
	 COMMAND echo ${PROJECT_BINARY_DIR}
     COMMENT "build main OUTPUT "
           )

ADD_CUSTOM_TARGET(t1 ALL DEPENDS o1)
ADD_DEPENDENCIES(server t1)
##################################
