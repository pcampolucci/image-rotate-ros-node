cmake_minimum_required(VERSION 2.8.3)
project(image_rotate)

# ----- PACKAGE DEPENDENCIES -----
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  cv_bridge
  sensor_msgs
  dynamic_reconfigure
)

# ----- ADD DYN RECONFIGURE API -----
#add dynamic reconfigure api
generate_dynamic_reconfigure_options(
  cfg/rotate.cfg
)

# ----- PACKAGE PROPERTIES -----
catkin_package(
  # Specify dependencies for when other packages depend on this package
  CATKIN_DEPENDS roscpp std_msgs sensor_msgs cv_bridge
  LIBRARIES visiontransfer
)

# ----- BUILD -----
find_package( OpenCV REQUIRED )
include_directories(${catkin_INCLUDE_DIRS})

# ----- MAKE EXECUTABLE -----
add_executable(
    rotate_node
    src/rotate.cpp
)

# ----- LINK LIBRARIES -----
target_link_libraries( rotate_node ${catkin_LIBRARIES} ) 
target_link_libraries( rotate_node ${OpenCV_LIBS} )

# make sure configure headers are built before any node using them
add_dependencies(rotate_node ${image_rotate}_gencfg)
