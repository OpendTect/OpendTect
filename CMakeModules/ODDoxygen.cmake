OPTION( BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

IF ( BUILD_DOCUMENTATION )
  FIND_PACKAGE( Doxygen )
  IF ( NOT DOXYGEN_FOUND )
    MESSAGE( FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  ENDIF()
ENDIF()

MACRO( OD_BUILD_DOCUMENTATION )
    SET( OD_DOXYGEN_PATH ${PROJECT_BINARY_DIR}/doc/Programmer/Generated/cmake )
    SET( OD_DOXYGEN_FILE ${OD_DOXYGEN_PATH}/Doxyfile )

    FOREACH ( MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
	SET ( OD_DOXYGEN_INPUTS "${OD_DOXYGEN_INPUTS} ${CMAKE_SOURCE_DIR}/include/${MODULE}" )
    ENDFOREACH()

    #-- Configure the Template Doxyfile for our specific project
    configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/templates/Doxyfile.in 
		 ${OD_DOXYGEN_FILE} @ONLY IMMEDIATE)

    #-- Add a custom target to run Doxygen when ever the project is built
    add_custom_target (   docs 
			COMMAND ${DOXYGEN_EXECUTABLE} ${OD_DOXYGEN_FILE}
			SOURCES ${OD_DOXYGEN_FILE} )
ENDMACRO()

