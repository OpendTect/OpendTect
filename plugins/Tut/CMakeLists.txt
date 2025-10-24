#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS VolumeProcessing )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
if ( NOT "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( OD_USEQT Core )
endif()
set( OD_MODULE_SOURCES
	tutexternalattribrandom.cc
	tutlogtools.cc
	tuthortools.cc
	tutorialattrib.cc
	tutseistools.cc
	tutvolproc.cc
	tutpi.cc
)
set( OD_MODULE_BATCHPROGS tut_process_batch.cc )
set( OD_MODULE_PROGS tut_simple_prog.cc )
set( OD_PLUGIN_ALO_EXEC
	${OD_ATTRIB_EXECS}
	${OD_VOLUME_EXECS}
	tut_process_batch
	tut_simple_prog )

if ( "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( OD_NO_ALO_ENTRY TRUE )  # OpendTect SDK internal build
endif()

OD_INIT_MODULE()

if ( NOT "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( PLUGIN_VERSION ${${OD_MODULE_NAME}_VERSION_MAJOR}.${${OD_MODULE_NAME}_VERSION_MINOR}.${${OD_MODULE_NAME}_VERSION_PATCH} )
    string( TOLOWER ${OD_MODULE_NAME} MOD_PREFIX )
    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/version.h.in"
                    "${CMAKE_CURRENT_BINARY_DIR}/${MOD_PREFIX}version.h" )
    unset( MOD_PREFIX )
    string( REPLACE ":\\" ":/" CMAKE_INSTALL_PREFIX_ed "${CMAKE_INSTALL_PREFIX}" )
    install( CODE "
              file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/relinfo/ver.${${OD_MODULE_NAME}_PACKAGE}_${OD_PLFSUBDIR}.txt\" \"${PLUGIN_VERSION}\n\" ) " )
    unset( CMAKE_INSTALL_PREFIX_ed )
    unset( PLUGIN_VERSION )

    if ( WIN32 AND NOT OD_NO_QT )
	add_custom_command( TARGET ${OD_MODULE_NAME} POST_BUILD
	    COMMAND ECHO [Paths] > qt.conf
	    COMMAND ECHO Prefix=\"${OD_BINARY_BASEDIR}/${OD_RUNTIME_DIRECTORY}/..\" >> qt.conf
	    WORKING_DIRECTORY "$<TARGET_FILE_DIR:${OD_MODULE_NAME}>"
	    COMMENT "Setting up qt.conf" )
    endif()
endif()
