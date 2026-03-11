cmake_minimum_required(VERSION 3.10)
project(Shoot)

set(CMAKE_CXX_STANDARD 14)

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# 添加可执行文件
add_executable(Shoot
  src/ui.cpp
  src/Renderer.cpp
  src/Enemy.cpp
  glad/glad.c
)

# 包含目录
target_include_directories(Shoot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Shoot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(Shoot PRIVATE ${GLFW_INCLUDE_DIRS})

# 链接库
target_link_libraries(Shoot PRIVATE ${GLFW_LIBRARIES} GL dl)
