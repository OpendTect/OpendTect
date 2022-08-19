#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(hdf5_test)
enable_language(CXX)

find_package(HDF5 REQUIRED)

include_directories(${HDF5_INCLUDE_DIRS})
set(hdf5_c_libs ${HDF5_LIBRARIES})
set(hdf5_cxx_libs ${HDF5_LIBRARIES} hdf5_cpp)

add_executable(hdf_read_simple hdf_read_simple.cc)
add_executable(hdf_write_read_simple hdf_write_read_simple.c)
target_link_libraries(hdf_write_read_simple ${hdf5_c_libs})
target_link_libraries(hdf_read_simple ${hdf5_cxx_libs})
