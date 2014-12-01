#_______________________Pmake___________________________________________________
#
#	Makefile: 	MATLAB plugin
# 	Feb 2012	Nanne
#	RCS:		$Id$
#_______________________________________________________________________________

set( MATLAB_DIR "" CACHE PATH "Directory containing MATLAB-SDK" )

if ( NOT MATLAB_DIR STREQUAL "" )

    set( OD_MODULE_DEPS VolumeProcessing )
    set( OD_IS_PLUGIN yes )
    set( OD_MODULE_SOURCES
	matlabarray.cc
	matlablibmgr.cc
	matlablinkpi.cc
	matlabstep.cc
    )

    list( APPEND OD_MODULE_INCLUDESYSPATH ${MATLAB_DIR}/extern/include )
    if ( UNIX )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${MATLAB_DIR}/bin/glnxa64/libmx.so ${MATLAB_DIR}/bin/glnxa64/libmwmclmcr.so )
    elseif ( WIN32 )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${MATLAB_DIR}/bin/${OD_PLFSUBDIR}/libmx.dll )
    endif()

    add_definitions( -DHAS_MATLAB )

    set( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )

    OD_INIT_MODULE()

endif()
