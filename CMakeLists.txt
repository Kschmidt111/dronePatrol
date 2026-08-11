cmake_minimum_required(VERSION 3.20)
project(dronePatrol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(dronePatrol src/main.cpp)

# Optional later:
# find_package(OpenCV REQUIRED)
# target_link_libraries(dronePatrol PRIVATE OpenCV::opencv_core OpenCV::opencv_imgproc OpenCV::opencv_dnn OpenCV::opencv_videoio OpenCV::opencv_highgui)
