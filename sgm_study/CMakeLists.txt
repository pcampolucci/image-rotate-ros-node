cmake_minimum_required(VERSION 3.0)

project(rotate)

find_package( OpenCV REQUIRED )

add_executable(
    rotate 
    image_rotate.cpp
)

target_link_libraries( rotate ${OpenCV_LIBS} )