# Locate Intel User defined Synchronization API
# This module defines
# ITTNOTIFY_LIBRARY
# ITTNOTIFY_FOUND, if false, do not try to link to zlib 
# ITTNOTIFY_INCLUDE_DIR, where to find the headers
#
# $ITTNOTIFY_DIR is an environment variable that would
# correspond to the installation directory of the ITTNOTIFY Tools
#
# Created by Kristofer Tingdahl, Copyright dGB Beheer B. V. 
#
# $Id: FindIttNotify.cmake 30343 2013-06-18 06:54:22Z kristofer.tingdahl@dgbes.com $
#

find_path(ITTNOTIFY_INCLUDE_DIR ittnotify.h
    $ENV{ITTNOTIFY_DIR}/include
    $ENV{ITTNOTIFY_DIR}
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include # Fink
    /opt/local/include # DarwinPorts
    /opt/csw/include # Blastwave
    /opt/include
    /usr/freeware/include
    ${ITTNOTIFY_DIR}/include
)

if ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set(LIBSUBDIR lib64)
else()
    set(LIBSUBDIR lib32)
endif()


find_library(ITTNOTIFY_LIBRARY 
    NAMES ittnotify
    PATHS
    $ENV{ITTNOTIFY_DIR}/${LIBSUBDIR}
    /usr/local/${LIBSUBDIR}
    /usr/${LIBSUBDIR}
    /sw/${LIBSUBDIR}
    /opt/local/${LIBSUBDIR}
    /opt/csw/${LIBSUBDIR}
    /opt/${LIBSUBDIR}
    /usr/freeware/${LIBSUBDIR}
    ${ITTNOTIFY_DIR}/${LIBSUBDIR}
)

set(ITTNOTIFY_FOUND "NO")
if(ITTNOTIFY_LIBRARY AND ITTNOTIFY_INCLUDE_DIR)
    set(ITTNOTIFY_FOUND "YES")
endif(ITTNOTIFY_LIBRARY AND ITTNOTIFY_INCLUDE_DIR)


