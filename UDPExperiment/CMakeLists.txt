# CMakeList.txt : CMake project for UDPExperiment, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (UDPExperiment "UDPExperiment.cpp" "UDPExperiment.h" "delta.h"  "packet.h" "packet.cpp" "delta.cpp" "snapshot.h" "object.h" "snapshot.cpp")

# TODO: Add tests and install targets if needed.
