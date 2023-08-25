#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to make release packages
#

if ( POLICY CMP0011 )
    cmake_policy( SET CMP0011 NEW )
endif()
if ( POLICY CMP0057 )
    cmake_policy( SET CMP0057 NEW )
endif()

if ( "${OpendTect_DIR}" STREQUAL "" )
    set( OpendTect_DIR "${SOURCE_DIR}" )
endif()

if( NOT DEFINED CMAKE_INSTALL_PREFIX )
    message( FATAL_ERROR "CMAKE_INSTALL_PREFIX is not defined." )
elseif( NOT IS_DIRECTORY "${CMAKE_INSTALL_PREFIX}" )
    message( FATAL_ERROR "${CMAKE_INSTALL_PREFIX} does not exist. Project may not be installed" )
endif()

find_program( ZIP_EXEC "7z" NAMES "7za"
	HINTS "C:/Program Files/7-Zip" "/usr/local/bin" 
	NO_CACHE )
if ( NOT ZIP_EXEC OR NOT EXISTS "${ZIP_EXEC}" )
    message( FATAL_ERROR "'7z/7za' executable is not installed or not in the path. Unable to create packages." )
endif()

if ( NOT DEFINED OD_PLFSUBDIR )
    message( FATAL_ERROR "OD_PLFSUBDIR not defined" )
endif()

include( ${OpendTect_DIR}/CMakeModules/packagescripts/ODMakePackagesUtils.cmake )
if ( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
    include( ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
else()
    message( FATAL_ERROR "File packages.cmake not found" )
endif()

if ( NOT PACKAGELIST OR "${PACKAGELIST}" STREQUAL "" )
    message( FATAL_ERROR "Empty list of packages provided" )
endif()

set( FULLVER_NAME "${OpendTect_FULL_VERSION}" )
set( REL_DIR "${OpendTect_INST_DIR}" )
if( APPLE )
    set( REL_DIR "OpendTect\ ${REL_DIR}.app" )
endif()

foreach ( PACKAGE ${PACKAGELIST} )
    if( "${PACKAGE}" STREQUAL "doc" OR "${PACKAGE}" STREQUAL "dgbdoc" OR
	"${PACKAGE}" STREQUAL "classdoc" )
	set( PACK ${PACKAGE} )
    else()
	if( EXISTS ${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	   include( ${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	elseif( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	   include( ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	else()
	    message( FATAL_ERROR "Configuration file not found for package ${PACKAGE}" )
	endif()
    endif()

    INIT_DESTINATIONDIR( ${PACK} )
    if ( "${PACK}" MATCHES "basedata$" )
	CREATE_BASEPACKAGE( ${PACK} )
    elseif( "${PACK}" MATCHES "doc$" )
	CREATE_DOCPACKAGE( ${PACK} )
    else()
	CREATE_PACKAGE( ${PACK} )
    endif()

    ZIPPACKAGE( "${PACKAGE_FILENAME}" "${REL_DIR}" "${PACKAGE_DIR}" )
    CLEAN_PACK_VARIABLES()
endforeach()
unset( PACKAGELIST )

if( OD_ENABLE_BREAKPAD )
    set( SYMBOLDIRNM symbols_${OD_PLFSUBDIR}_${OpendTect_FULL_VERSION} )
    #no file(COPY), for as long as the source path is configuration dependent
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}/symbols"
		     "${PACKAGE_DIR}/symbols/${SYMBOLDIRNM}" )
    ZIPPACKAGE( "${SYMBOLDIRNM}.zip" "${SYMBOLDIRNM}" "${PACKAGE_DIR}/symbols" )
endif()

message( STATUS "Created packages are available at ${PACKAGE_DIR}" )
