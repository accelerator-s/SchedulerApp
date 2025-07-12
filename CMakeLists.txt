cmake_minimum_required(VERSION 3.10)
project(SchedulerApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 查找依赖包 ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
pkg_check_modules(APPINDICATOR REQUIRED ayatana-appindicator3-0.1)

# --- 设置头文件搜索路径 ---
include_directories(src) 
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${APPINDICATOR_INCLUDE_DIRS})

# --- 设置输出路径 ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE})

# --- 添加可执行文件 ---
# 列出所有源文件
add_executable(SchedulerApp src/main.cpp src/SchedulerApp.cpp src/UserManager.cpp src/TaskManager.cpp src/md5.cpp)

# --- 链接库 ---
target_link_libraries(SchedulerApp PRIVATE 
    ${GTKMM_LIBRARIES} 
    Threads::Threads
    OpenSSL::Crypto
    ${APPINDICATOR_LIBRARIES}
)

# 自定义文件复制函数
add_custom_command(
    TARGET SchedulerApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/GUIDesign.xml"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/GUIDesign.xml"
    COMMENT "Copying GUIDesign.xml to output directory..."
)


# Gtkmm需要的一些额外编译定义
target_compile_definitions(SchedulerApp PRIVATE ${GTKMM_DEFINITIONS})

# 可选：添加编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(SchedulerApp PRIVATE -Wall -Wextra -pedantic)
endif()