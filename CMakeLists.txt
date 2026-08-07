cmake_minimum_required(VERSION 4.0)
project(recren)

set(CMAKE_CXX_STANDARD 23)

add_executable(recren
        src/main.cpp
)

find_package(OpenCV CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_library(utils src/utils.cpp)
target_link_libraries(utils
        PUBLIC
        glad::glad
        OpenGL::GL
        opencv_core
        opencv_calib3d
        glfw
)

add_library(renderer src/renderer.cpp)
target_link_libraries(renderer
        PUBLIC
        utils
)

target_link_libraries(recren
        PRIVATE
        renderer
        opencv_imgproc
        opencv_highgui
        opencv_videoio
)