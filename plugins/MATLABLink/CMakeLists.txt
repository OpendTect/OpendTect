#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

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
    if ( APPLE )
    elseif ( WIN32 )
	list( APPEND OD_MODULE_EXTERNAL_SYSLIBS
	        ${MATLAB_DIR}/extern/lib/win64/microsoft/libmx.lib
		${MATLAB_DIR}/extern/lib/win64/microsoft/mclbase.lib
		${MATLAB_DIR}/extern/lib/win64/microsoft/mclmcr.lib )
    else()
	list( APPEND OD_MODULE_EXTERNAL_SYSLIBS
	    ${MATLAB_DIR}/bin/glnxa64/libmx.so
	    ${MATLAB_DIR}/bin/glnxa64/libmwmclmcr.so )
    endif()
    add_definitions( -DHAS_MATLAB )
    if ( WIN32 )
	add_definitions( -D_XKEYCHECK_H )
    endif()

    set( OD_PLUGIN_ALO_EXEC ${OD_VOLUME_EXECS} )

    OD_INIT_MODULE()

    if ( EXISTS ${MATLAB_DIR}/bin/mex )
# Macro for building mex files
	macro ( MATLAB_EXEC_MEX PROG ARGS OUTDIR WORKDIR)
	    add_custom_command(
		    TARGET ${OD_MODULE_NAME} POST_BUILD
		    COMMAND ${MATLAB_DIR}/bin/mex
		    ARGS ${ARGS} -outdir ${OUTDIR} ${WORKDIR}/${PROG}
						   ${WORKDIR}/matlabarray.cc
		    WORKING_DIRECTORY ${WORKDIR}
		    COMMENT "Mex-ing ${PROG}" VERBATIM )
	endmacro()

	set( MATLAB_PROGS
	    readcbvs.cc
	    writecbvs.cc
	    writesegyindex.cc
	)

	set( MATLAB_PROG_DEPS Basic Algo General Geometry
			      MMProc Network NLA Seis AttributeEngine )
	set( ARGS -D__${OD_PLFSUBDIR}__ )
	foreach( MOD ${MATLAB_PROG_DEPS} )
	    foreach( INC ${OD_${MOD}_INCLUDEPATH} )
		list( APPEND ARGS -I${INC} )
	    endforeach()
	endforeach()

	set( ARGS ${ARGS} -L${OD_EXEC_OUTPUT_PATH} )
	foreach( MOD ${MATLAB_PROG_DEPS} )
	    list( APPEND ARGS -l${MOD} )
	endforeach()

	set( OUTDIR ${OpendTect_DIR}/bin/${OD_PLFSUBDIR}/MATLAB )
	if ( NOT EXISTS ${OUTDIR} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTDIR} )
	endif()

	set( WORKDIR ${OpendTect_DIR}/plugins/${OD_MODULE_NAME} )
	foreach ( PROG ${MATLAB_PROGS} )
	    MATLAB_EXEC_MEX( "${PROG}" "${ARGS}" "${OUTDIR}" "${WORKDIR}" )
	endforeach()

	install( DIRECTORY ${OUTDIR} DESTINATION
		 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR} )
    endif()
else()
    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_PLUGINS ${OD_MODULE_NAME} )
    set( OD_PLUGINS ${OD_PLUGINS} PARENT_SCOPE )
endif()

