cmake_minimum_required(VERSION 3.10)
project(SchedulerApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 查找依赖包 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)  # 添加OpenSSL依赖

# --- 设置头文件搜索路径 ---
include_directories(src) 
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})  # 添加OpenSSL头文件路径

# --- 设置输出路径 ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE})

# 将UI文件复制到相应的输出目录
install(FILES GUIDesign.xml 
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# --- 添加可执行文件 ---
add_executable(SchedulerApp src/main.cpp src/SchedulerApp.cpp src/UserManager.cpp src/TaskManager.cpp src/md5.cpp)

# --- 链接库 ---
target_link_libraries(SchedulerApp PRIVATE 
    ${GTKMM_LIBRARIES} 
    Threads::Threads
    OpenSSL::Crypto  # 添加OpenSSL加密库
)

# Gtkmm需要的一些额外编译定义
target_compile_definitions(SchedulerApp PRIVATE ${GTKMM_DEFINITIONS})

# 可选：添加编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(SchedulerApp PRIVATE -Wall -Wextra -pedantic)
endif()