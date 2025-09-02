#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define devel package variables
#

#OpendTect modules and plugins
set( LIBLIST ${OD_CORE_MODULES} )

set( PLUGINS ${OD_CORE_PLUGINS} )
set( EXCLUDE_PLUGINS Tut )
list( REMOVE_ITEM PLUGINS ${EXCLUDE_PLUGINS} )

set( EXECLIST ${OD_CORE_EXECUTABLES} )
set( EXCLUDE_EXECS tut_process_batch tut_simple_prog )
list( REMOVE_ITEM EXECLIST ${EXCLUDE_EXECS} )

set( SPECMODS ${OD_CORE_SPECMODS} )

if ( ${PACKAGE_TYPE} STREQUAL "Production" )

    set( COPY_TARGETS FALSE )

else()

    foreach( TRGT ${OD_THIRDPARTY_TARGETS} )
	if ( "${TRGT}" MATCHES "^Qt.*::" )
	    if ( "${TRGT}" MATCHES "^Qt.*::Core$" OR
		 "${TRGT}" MATCHES "^Qt.*::DBus$" OR
		 "${TRGT}" MATCHES "^Qt.*::Network$" OR
		 "${TRGT}" MATCHES "^Qt.*::Sql$" )
		list( APPEND THIRDPARTY_TARGETS ${TRGT} )
	    endif()
	elseif ( NOT "${TRGT}" MATCHES "^osg::" AND
		 NOT "${TRGT}" MATCHES "^Fontconfig::" AND
		 NOT "${TRGT}" MATCHES "^Freetype::" AND
		 NOT "${TRGT}" MATCHES "^PNG::" )
	    list( APPEND THIRDPARTY_TARGETS ${TRGT} )
	endif()
    endforeach()

    foreach( LIB ${OD_THIRDPARTY_LIBS} )
	if ( "${LIB}" MATCHES "^${SHLIB_PREFIX}Qt.*${SHLIB_EXTENSION}.*" )
	    if ( "${LIB}" MATCHES "Qt.*Core" OR
		 "${LIB}" MATCHES "Qt.*DBus" OR
		 "${LIB}" MATCHES "Qt.*Network" OR
		 "${LIB}" MATCHES "Qt.*Sql" )
		list( APPEND THIRDPARTY_LIBS ${LIB} )
	    endif()
	else()
	    if ( NOT "${LIB}" MATCHES "^${SHLIB_PREFIX}osg.*${SHLIB_EXTENSION}.*" AND
		 NOT "${LIB}" MATCHES ".*OpenThreads.*${SHLIB_EXTENSION}.*" AND
		 NOT "${LIB}" MATCHES "^fontconfig" AND
		 NOT "${LIB}" MATCHES "^freetype" AND
		 NOT "${LIB}" MATCHES "^png" AND
		 NOT "${LIB}" MATCHES "^d3dcompiler" AND
		 NOT "${LIB}" MATCHES "^opengl32sw" )
		list( APPEND THIRDPARTY_LIBS ${LIB} )
	    endif()
	endif()
    endforeach()

    if ( WIN32 )
	unset( QTPLUGINS )
	set( QTPLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}/../plugins" )
	foreach( QTPLUGIN_FILE ${OD_QTPLUGINS} )
	    if ( "${QTPLUGIN_FILE}" MATCHES "^sqldrivers" OR
		 "${QTPLUGIN_FILE}" MATCHES "^tls" )
		cmake_path( GET QTPLUGIN_FILE PARENT_PATH QTPLUGIN_DIR )
		cmake_path( GET QTPLUGIN_FILE STEM QTPLUGIN_STEM )
		cmake_path( GET QTPLUGIN_FILE EXTENSION QTPLUGIN_FILEEXT )
		if ( EXISTS "${QTPLUGINS_DIR}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}" )
		    list( APPEND QTPLUGINS "${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}" )
		endif()
	    endif()
	endforeach()
	unset( QTPLUGINS_DIR )
    endif()

endif()

set( PACK devel )
if ( ${PACKAGE_TYPE} STREQUAL "Production" )
    set( PACK "${PACK}rel" )
endif()
