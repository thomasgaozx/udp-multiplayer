# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("UDPExperiment")

# Include sub-projects.
add_subdirectory ("UDPExperiment")

find_path(BOOST_ASIO_INCLUDE_DIRS "boost/asio.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${BOOST_ASIO_INCLUDE_DIRS})


# find_package(Boost REQUIRED COMPONENTS system)
# include_directories( ${Boost_INCLUDE_DIR} )

# target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

# add_executable( anyExecutable UDPExperiment/UDPExperiment.cpp )

# target_link_libraries(UDPExperiment PRIVATE ${Boost_FILESYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY})
