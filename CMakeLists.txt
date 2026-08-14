cmake_minimum_required(VERSION 3.20)
project(dronePatrol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio highgui dnn)

add_executable(dronePatrol
    src/main.cpp
    src/capture.cpp
    src/detector.cpp
)

target_include_directories(dronePatrol PRIVATE include)
target_link_libraries(dronePatrol PRIVATE ${OpenCV_LIBS})

# Copy OpenCV / FFmpeg DLLs next to the exe on Windows (CMake 3.21+).
if(WIN32)
    add_custom_command(TARGET dronePatrol POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:dronePatrol>
            $<TARGET_FILE_DIR:dronePatrol>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs next to dronePatrol.exe"
    )
endif()
