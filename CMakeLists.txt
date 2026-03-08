cmake_minimum_required(VERSION 3.10)
project(Powder)

set(CMAKE_CXX_STANDARD 11)

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# 添加可执行文件
add_executable(Powder
  src/ui.cpp
  src/renderer.cpp
  glad/glad.c
)

# 包含目录
target_include_directories(Powder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Powder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(Powder PRIVATE ${GLFW_INCLUDE_DIRS})

# 链接库
target_link_libraries(Powder PRIVATE ${GLFW_LIBRARIES} GL dl)
