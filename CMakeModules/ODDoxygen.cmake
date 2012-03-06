#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODDoxygen.cmake,v 1.2 2012-03-06 11:23:04 cvskris Exp $
#_______________________________________________________________________________

OPTION( BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

# OD_BUILD_DOCUMENTATION - Make target "doc" to make documentation
MACRO( OD_BUILD_DOCUMENTATION )
    SET( OD_DOXYGEN_PATH ${PROJECT_BINARY_DIR}/doc/Programmer/Generated/cmake )
    SET( OD_DOXYGEN_FILE ${OD_DOXYGEN_PATH}/Doxyfile )

    FOREACH ( OD_DOXYGEN_MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
	SET ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${CMAKE_SOURCE_DIR}/include/${OD_DOXYGEN_MODULE} ${CMAKE_SOURCE_DIR}/src/${OD_DOXYGEN_MODULE}" )
    ENDFOREACH()

    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Doxyfile.in 
		 ${OD_DOXYGEN_FILE} @ONLY IMMEDIATE)

    add_custom_target ( doc 
			COMMAND ${DOXYGEN_EXECUTABLE} ${OD_DOXYGEN_FILE}
			SOURCES ${OD_DOXYGEN_FILE} )
ENDMACRO()

IF ( BUILD_DOCUMENTATION )
  FIND_PACKAGE( Doxygen )
  IF ( NOT DOXYGEN_FOUND )
    MESSAGE( FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  ENDIF()
ENDIF()

