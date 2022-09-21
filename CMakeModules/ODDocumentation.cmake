#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

OPTION( BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

file ( GLOB DOCUMENTATION_FILES "${CMAKE_SOURCE_DIR}/doc/Programmer/*.html.in" )

foreach( DOCFILE ${DOCUMENTATION_FILES} ) 
    get_filename_component( OUTPUT ${DOCFILE} NAME_WE )
    configure_file( ${DOCFILE} ${PROJECT_BINARY_DIR}/doc/Programmer/${OUTPUT}.html @ONLY )
endforeach()


# OD_BUILD_DOCUMENTATION - Make target "doc" to make the class documentation
macro( OD_BUILD_DOCUMENTATION )
    set( OD_DOXYGEN_PATH ${PROJECT_BINARY_DIR}/doc/Programmer/Generated )
    set( OD_DOXYGEN_FILE ${PROJECT_BINARY_DIR}/CMakeModules/Doxyfile )
    set( OD_DOXYGEN_LOGO ${CMAKE_SOURCE_DIR}/doc/Programmer/images/logo.png )

    if ( NOT EXISTS "${OD_DOXYGEN_PATH}" )
	file(MAKE_DIRECTORY "${OD_DOXYGEN_PATH}" )
    endif()

    foreach ( OD_DOXYGEN_MODULE ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}} )
	set( INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${OD_DOXYGEN_MODULE} )
	if ( EXISTS ${INCLUDE_DIR} )
	    set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${INCLUDE_DIR}" )
	    file ( GLOB DOXFILES ${INCLUDE_DIR}/*.dox )
	    foreach ( DOX_FILE ${DOXFILES} )
		if ( EXISTS ${DOX_FILE} )
		    OD_ADD_SOURCE_FILES( ${DOX_FILE} )
		    set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${DOX_FILE}" )
		endif()
	    endforeach()
	endif()
	set( SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/${OD_DOXYGEN_MODULE} )
	if( EXISTS ${SOURCE_DIR} )
	    set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${SOURCE_DIR}" )
	endif()
    endforeach()

    set( PLUGIN_DIR ${CMAKE_SOURCE_DIR}/plugins )
    if( EXISTS ${PLUGIN_DIR} )
	set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${PLUGIN_DIR}" )
    endif()

    set( TEMPLATE ${OpendTect_DIR}/CMakeModules/templates/Doxyfile.in )
    set( FOOTER ${OpendTect_DIR}/CMakeModules/templates/doxygenfooter.html.in )
	
    configure_file( ${TEMPLATE}
		 ${OD_DOXYGEN_FILE} @ONLY IMMEDIATE)
    OD_CURRENT_YEAR( YEAR )
    configure_file( ${FOOTER}
		${PROJECT_BINARY_DIR}/CMakeFiles/doxygenfooter.html @ONLY
		IMMEDIATE)

    OD_ADD_SOURCE_FILES( ${TEMPLATE} ${FOOTER} )

    add_custom_target ( doc
			COMMAND ${DOXYGEN_EXECUTABLE} ${OD_DOXYGEN_FILE}
			SOURCES ${OD_DOXYGEN_FILE} )

    install ( DIRECTORY ${CMAKE_BINARY_DIR}/doc/Programmer/Generated/html
	      DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer/Generated )
endmacro()

IF ( BUILD_DOCUMENTATION )
  find_package( Doxygen )
  if ( NOT DOXYGEN_FOUND )
    message( FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()
endif()

if ( WIN32 )
    option( BUILD_USERDOC "Build user documentation" OFF )
endif()

# OD_BUILD_USERDOCUMENTATION - Make target "userdoc" to make software documentation
macro( OD_BUILD_USERDOCUMENTATION )
    set( USERDOC_PROJECT "" CACHE FILEPATH "Path to user documentation project" )
    set( USERDOC_TARGET "HTML" CACHE STRING "Documentation target" )
    find_program( MADCAP_FLARE_EXEC madbuild.exe
		  PATHS
		    "C:/Program Files/MadCap Software/"
		    "D:/Program Files/MadCap Software/"
		    "E:/Program Files/MadCap Software/"
		  PATH_SUFFIXES
		    "MadCap Flare 16/Flare.app"
		    "MadCap Flare 17/Flare.app"
		    "MadCap Flare 18/Flare.app"
		    "MadCap Flare 19/Flare.app"
		  DOC "Madcap Flare Executable"
             	  NO_DEFAULT_PATH )

    if ( NOT EXISTS "${MADCAP_FLARE_EXEC}" )
	message ( FATAL_ERROR "Madcap flare executable not found" )
    endif()

    if ( NOT EXISTS "${USERDOC_PROJECT}" )
	message ( FATAL_ERROR "Cannot find ${USERDOC_PROJECT}" )
    endif()

    set ( USER $ENV{USERNAME} )
    get_filename_component( USERDOC_PROJECT_DIR "${USERDOC_PROJECT}" PATH )
    get_filename_component( USERDOC_NAME "${USERDOC_PROJECT}" NAME_WE )

    set ( USERDOC_OUTPUT_DIR "${USERDOC_PROJECT_DIR}/Output/${USER}/${USERDOC_TARGET}" )

    add_custom_target( "userdoc" "${MADCAP_FLARE_EXEC}"
			-project "${USERDOC_PROJECT}"
			-target "${USERDOC_TARGET}"
			COMMENT "Building user documentation" )
    install( DIRECTORY "${USERDOC_OUTPUT_DIR}/" DESTINATION "${MISC_INSTALL_PREFIX}/doc/${USERDOC_NAME}" )

endmacro( OD_BUILD_USERDOCUMENTATION )
