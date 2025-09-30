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

find_program( ZIP_EXEC "7z" NAMES "7za" "7zz"
		HINTS "C:/Program Files/7-Zip"
		      "/usr/local/bin"
		      "/opt/homebrew/bin"
		NO_CACHE )
if ( NOT ZIP_EXEC OR NOT EXISTS "${ZIP_EXEC}" )
    message( FATAL_ERROR "'7z/7za/7zz' executable is not installed or not in the path. Unable to create packages." )
endif()

if ( NOT DEFINED OD_PLFSUBDIR )
    message( FATAL_ERROR "OD_PLFSUBDIR not defined" )
endif()

include( ${OpendTect_DIR}/CMakeModules/packagescripts/ODMakePackagesUtils.cmake )
if ( EXISTS ${SOURCE_DIR}/cmake/packagescripts/packages.cmake )
    include( ${SOURCE_DIR}/cmake/packagescripts/packages.cmake )
elseif ( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake )
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

if( NOT EXISTS "${PACKAGE_DIR}" )
    file( MAKE_DIRECTORY "${PACKAGE_DIR}" )
endif()

set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
if ( APPLE )
    set( MISC_INSTALL_PREFIX "/Contents/Resources" )
endif()

set( COPYFROMDIR "${CMAKE_INSTALL_PREFIX}${MISC_INSTALL_PREFIX}" )
set( COPYTODIR "${DESTINATION_DIR}${MISC_INSTALL_PREFIX}" )
set( COPYFROMSCRIPTSDIR "${COPYFROMDIR}/bin" )
set( COPYTOSCRIPTSDIR "${COPYTODIR}/bin" )
set( COPYFROMDATADIR "${COPYFROMDIR}/share" )
set( COPYTODATADIR "${COPYTODIR}/share" )
set( COPYFROMLIBDIR "${CMAKE_INSTALL_PREFIX}/${OD_LIBRARY_DIRECTORY}" )
set( COPYTOLIBDIR "${DESTINATION_DIR}/${OD_LIBRARY_DIRECTORY}" )
set( COPYFROMLOCDIR "${CMAKE_INSTALL_PREFIX}/${OD_LOCATION_DIRECTORY}" )
set( COPYTOLOCDIR "${DESTINATION_DIR}/${OD_LOCATION_DIRECTORY}" )
set( COPYFROMEXEDIR "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}" )
set( COPYTOEXEDIR "${DESTINATION_DIR}/${OD_RUNTIME_DIRECTORY}" )

foreach ( PACKAGE ${PACKAGELIST} )
    if( NOT APPLE AND EXISTS "${BINARY_DIR}/cmake/packagescripts/${PACKAGE}.cmake" )
       include( "${BINARY_DIR}/cmake/packagescripts/${PACKAGE}.cmake" )
    elseif( NOT APPLE AND EXISTS "${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake" )
       include( "${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake" )
    elseif( APPLE AND EXISTS "${BINARY_DIR}/Contents/Resources/cmake/packagescripts/${PACKAGE}.cmake" )
       include( "${BINARY_DIR}/Contents/Resources/cmake/packagescripts/${PACKAGE}.cmake" )
    elseif( APPLE AND EXISTS "${BINARY_DIR}/Contents/Resources/CMakeModules/packagescripts/${PACKAGE}.cmake" )
       include( "${BINARY_DIR}/Contents/Resources/CMakeModules/packagescripts/${PACKAGE}.cmake" )
    elseif( EXISTS "${SOURCE_DIR}/cmake/packagescripts/${PACKAGE}.cmake" )
       include( "${SOURCE_DIR}/cmake/packagescripts/${PACKAGE}.cmake" )
    elseif( EXISTS "${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake" )
       include( "${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake" )
    else()
	message( FATAL_ERROR "Configuration file not found for package ${PACKAGE}" )
    endif()
    if ( NOT DEFINED ISBASE )
	set( ISBASE FALSE )
    endif()
    if ( NOT DEFINED ISUSERDOC )
	set( ISUSERDOC FALSE )
    endif()
    if ( NOT DEFINED ISCLASSDOC )
	set( ISCLASSDOC FALSE )
    endif()
    if ( NOT DEFINED ISDEVEL )
	set( ISDEVEL FALSE )
    endif()

    INIT_DESTINATIONDIR( ${PACK} )
    CREATE_PACKAGE( ${PACK} )

    ZIPPACKAGE( "${PACKAGE_FILENAME}" "${REL_DIR}" "${PACKAGE_DIR}" )
    if( ISCLASSDOC )
	if ( EXISTS "${CLASSDOC_SCRIPT_LOCATION}" )
	    message( STATUS "Preparing package ${PACK}_mac ....." )
	    execute_process( COMMAND "${CLASSDOC_SCRIPT_LOCATION}"
				    --reldir "${OpendTect_INST_DIR}"
				    --ver ${FULLVER_NAME}
			     WORKING_DIRECTORY "${PACKAGE_DIR}"
			     RESULT_VARIABLE STATUS
			     OUTPUT_QUIET
			     ERROR_VARIABLE ERRVAR )
	    if ( NOT ${STATUS} EQUAL 0 )
		message( SEND_ERROR "Failed to create macOS ${PACK} package: ${STATUS} - ${ERRVAR}" )
	    endif()
	else()
	    message( SEND_ERROR "Cannot create macOS ${PACK} package without the script location" )
	endif()
    elseif( ISUSERDOC )
	if ( EXISTS "${USERDOC_SCRIPT_LOCATION}" )
	    message( STATUS "Preparing package ${PACK}_mac ....." )
	    execute_process( COMMAND "${USERDOC_SCRIPT_LOCATION}"
				    ${PACK} --reldir "${OpendTect_INST_DIR}"
			     WORKING_DIRECTORY "${PACKAGE_DIR}"
			     RESULT_VARIABLE STATUS
			     OUTPUT_QUIET
			     ERROR_VARIABLE ERRVAR )
	    if ( NOT ${STATUS} EQUAL 0 )
		message( SEND_ERROR "Failed to create macOS ${PACK} package: ${STATUS} - ${ERRVAR}" )
	    endif()
	else()
	    message( SEND_ERROR "Cannot create macOS ${PACK} package without the script location" )
	endif()
    endif()
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
