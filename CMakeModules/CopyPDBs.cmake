foreach( DLLFILE ${DLLFILES} )
    get_filename_component( DLL_DIR "${DLLFILE}" DIRECTORY )
    get_filename_component( DLL_NAMEWE "${DLLFILE}" NAME_WE )
    set( PDB_FILE "${DLL_DIR}/${DLL_NAMEWE}.pdb" )
    if ( EXISTS "${PDB_FILE}" AND
	 NOT "${DLL_DIR}" STREQUAL "${DESTINATION}" )
	list( APPEND PDB_FILES "${PDB_FILE}" )
    endif()
endforeach()

if ( NOT "${PDB_FILES}" STREQUAL "" )
    file( COPY ${PDB_FILES}
	  DESTINATION "${DESTINATION}" )
endif()
