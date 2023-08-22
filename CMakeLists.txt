cmake_minimum_required(VERSION 3.25)
project(vulkanDemo)

set(CMAKE_CXX_STANDARD 17)

# Include directories for headers
include_directories(C:/VulkanSDK/1.3.250.1/Include)
include_directories(C:/Users/xiuyuan.zhu/Lib)
include_directories(C:/Users/xiuyuan.zhu/Lib/glm)
include_directories(C:/Users/xiuyuan.zhu/Lib/glfw-3.3.8.bin.WIN64/include)

# Link directories for libraries
link_directories(C:/VulkanSDK/1.3.250.1/Lib)
link_directories(C:/Users/xiuyuan.zhu/Lib/glfw-3.3.8.bin.WIN64/lib-mingw-w64)

# Link libraries
link_libraries(vulkan-1)
link_libraries(glfw3)

add_executable(vulkanDemo main.cpp)
