cmake_minimum_required(VERSION 3.10)
project(SchedulerApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 查找依赖包 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
find_package(Threads REQUIRED) # 查找并加载线程库

# --- 设置头文件搜索路径 ---
# 将 src 目录添加到头文件搜索路径中
include_directories(src) 
# GTKmm 3.0 的头文件目录
include_directories(${GTKMM_INCLUDE_DIRS})

# --- 设置输出路径 ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE})

# 将UI文件复制到相应的输出目录
file(COPY GUIDesign.xml DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# --- 添加可执行文件 ---
add_executable(SchedulerApp src/main.cpp src/SchedulerApp.cpp src/UserManager.cpp src/TaskManager.cpp src/md5.cpp)

# --- 链接库 ---
target_link_libraries(SchedulerApp PRIVATE ${GTKMM_LIBRARIES} Threads::Threads)

# Gtkmm需要的一些额外编译定义
target_compile_definitions(SchedulerApp PRIVATE ${GTKMM_DEFINITIONS})