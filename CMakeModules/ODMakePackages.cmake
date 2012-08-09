

macro ( create_package PACKAGE_NAME )
    string ( ${PACKAGE_NAME} PACKAGE_NAME_UPPER )
    set ( FILESLIST ${${PACKAGE_NAME_UPPER}_FILELIST} )

    set ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    if ( APPLE OR ${${PACKAGE_NAME_UPPER}_PLFDEP )
	set ( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${PLFSUBDIR}" )
    endif()
    set ( PACKAGE_FILENAME "${PACKAGE_FILENAME}.zip" )

    set ( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if ( EXISTS ${DESTINATION_DIR} )
	file ( REMOVE_RECURSE ${DESTINATION_DIR} )
    endif()
    file ( MAKE_DIRECTORY ${DESTINATION_DIR} )

    file ( INSTALL ${FILELIST} DESTINATION ${DESTINATION_DIR} )

    exececute_process( COMMAND "zip -ry ${PACKAGE_FILENAME} ${REL_DIR}" 
		       WORKING_DIRECTORY ${PACKAGE_DIR}
		       OUTPUT_VARIABLE STATUS )
    if ( NOT ${STATUS} EQUAL "0" )
	message( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
    endif()

endmacro( create_package )


set ( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
if ( APPLE )
    set ( REL_DIR "OpendTect${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.app" )
endif ( APPLE )

set ( DOC_FILELIST "doc" )
set ( DOC_PLFDEP )

set ( DEVEL_FILELIST "CMakeModules" )
if ( WIN32 )
    list ( APPEND DEVEL_FILELIST "/bin/${PLFSUBDIR}/Debug" )
endif ( WIN32 )
set ( DEVEL_PLFDEP 1 )

create_package( doc )
create_package( devel )
