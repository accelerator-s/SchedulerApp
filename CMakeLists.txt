cmake_minimum_required(VERSION 3.10)
project(SchedulerApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 构建选项 - 强制启用静态链接和资源嵌入
set(EMBED_RESOURCES ON CACHE BOOL "Embed resources into executable" FORCE)

# 静态链接设置（强制启用）
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# 尝试静态链接系统库
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

# --- 查找依赖包 ---
find_package(PkgConfig REQUIRED)

# 包配置
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(APPINDICATOR REQUIRED ayatana-appindicator3-0.1)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics audio)
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

# --- 资源嵌入 ---
set(RESOURCE_GENERATOR ${CMAKE_SOURCE_DIR}/src/generate_resources.cpp)
set(EMBEDDED_RESOURCES_H ${CMAKE_SOURCE_DIR}/src/embedded_resources.h)
set(EMBEDDED_RESOURCES_CPP ${CMAKE_SOURCE_DIR}/src/embedded_resources.cpp)

# 编译资源生成器
add_executable(resource_generator ${RESOURCE_GENERATOR})
target_compile_features(resource_generator PRIVATE cxx_std_17)

# 生成嵌入资源
add_custom_command(
    OUTPUT ${EMBEDDED_RESOURCES_H} ${EMBEDDED_RESOURCES_CPP}
    COMMAND resource_generator
    DEPENDS resource_generator ${CMAKE_SOURCE_DIR}/src/GUIDesign.xml ${CMAKE_SOURCE_DIR}/asserts/notification.mp3
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating embedded resources..."
)

# 设置源文件列表（包含嵌入的资源）
set(SOURCE_FILES 
    src/main.cpp 
    src/SchedulerApp.cpp 
    src/UserManager.cpp 
    src/TaskManager.cpp 
    src/md5.cpp
    ${EMBEDDED_RESOURCES_CPP}
)

# --- 添加可执行文件 ---
add_executable(SchedulerApp ${SOURCE_FILES})

# --- 链接库 ---
# 链接SFML（静态链接）
target_link_libraries(SchedulerApp PRIVATE sfml-audio sfml-graphics sfml-window sfml-system)

# 链接其他库（静态链接）
target_link_libraries(SchedulerApp PRIVATE 
    ${GTKMM_LIBRARIES} 
    Threads::Threads
    OpenSSL::Crypto
    ${APPINDICATOR_LIBRARIES}
)

# 静态链接标志（强制启用）
target_link_libraries(SchedulerApp PRIVATE 
    -static-libgcc
    -static-libstdc++
)

# 添加额外的链接选项以支持静态链接
target_link_options(SchedulerApp PRIVATE
    -Wl,--as-needed
    -Wl,--gc-sections
)

# 嵌入资源时的编译定义
target_compile_definitions(SchedulerApp PRIVATE EMBEDDED_RESOURCES_ENABLED)


# Gtkmm需要的一些额外编译定义
target_compile_definitions(SchedulerApp PRIVATE ${GTKMM_DEFINITIONS})

# 可选：添加编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(SchedulerApp PRIVATE -Wall -Wextra -pedantic)
endif()