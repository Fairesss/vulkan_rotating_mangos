cmake_minimum_required(VERSION 3.25)
project(fair_engine)

find_package(Vulkan)
find_package(glfw3)
find_package(glm)
find_package(Stb)
set(Stb_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/stb/include)
set(CMAKE_C_STANDARD 17)


add_executable(${PROJECT_NAME} src/main.cpp engine/src/App.cpp engine/headers/App.h
        engine/headers/SwapChain.h)

execute_process(
        COMMAND glslc ${PROJECT_SOURCE_DIR}/engine/shader/shader.vert -o ${PROJECT_SOURCE_DIR}/engine/shader/shader.vert.spv
        COMMAND echo compiled
)
message(DEBUG "vert shader compiled")
execute_process(
        COMMAND glslc ${PROJECT_SOURCE_DIR}/engine/shader/shader.frag -o ${PROJECT_SOURCE_DIR}/engine/shader/shader.frag.spv
)
message(DEBUG "frag shader compiled")

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
