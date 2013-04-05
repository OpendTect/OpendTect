#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

OPTION( BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

# OD_BUILD_DOCUMENTATION - Make target "doc" to make documentation
macro( OD_BUILD_DOCUMENTATION )
    set( OD_DOXYGEN_PATH ${PROJECT_BINARY_DIR}/doc/Programmer/Generated )
    set( OD_DOXYGEN_FILE ${OD_DOXYGEN_PATH}/Doxyfile )
    set( OD_DOXYGEN_INPUT "${CMAKE_SOURCE_DIR}/include/Basic/main.dox" )
    OD_ADD_SOURCE_FILES( ${CMAKE_SOURCE_DIR}/include/Basic/main.dox )

    foreach ( OD_DOXYGEN_MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
	set( INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${OD_DOXYGEN_MODULE} )
	string(TOLOWER ${OD_DOXYGEN_MODULE} OD_DOXYGEN_MODULE_lower )
	if ( EXISTS ${INCLUDE_DIR} )
	    set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${INCLUDE_DIR}" )
	    set( DOX_FILE ${INCLUDE_DIR}/${OD_DOXYGEN_MODULE_lower}.dox )
	    if ( EXISTS ${DOX_FILE} )
		OD_ADD_SOURCE_FILES( ${DOX_FILE} )
		set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${DOX_FILE}" )
	    endif()
	endif()
	set( SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/${OD_DOXYGEN_MODULE} )
	if( EXISTS ${SOURCE_DIR} )
	    set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${SOURCE_DIR}" )
	endif()
    endforeach()

    set( TEMPLATE ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Doxyfile.in )
    set( FOOTER ${CMAKE_SOURCE_DIR}/CMakeModules/templates/doxygenfooter.html.in )

    
    configure_file( ${TEMPLATE}
		 ${OD_DOXYGEN_FILE} @ONLY IMMEDIATE)
    OD_CURRENT_YEAR( YEAR )
    configure_file( ${FOOTER}
		 ${OD_DOXYGEN_PATH}/footer.html @ONLY IMMEDIATE)

    OD_ADD_SOURCE_FILES( ${TEMPLATE} ${FOOTER} )

    add_custom_target ( doc 
			COMMAND ${DOXYGEN_EXECUTABLE} ${OD_DOXYGEN_FILE}
			SOURCES ${OD_DOXYGEN_FILE} )
    install ( DIRECTORY doc/Programmer/Generated/html DESTINATION doc/Programmer/Generated )
endmacro()

IF ( BUILD_DOCUMENTATION )
  find_package( Doxygen )
  if ( NOT DOXYGEN_FOUND )
    message( FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()
endif()

