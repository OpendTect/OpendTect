#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

OPTION( BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)

file ( GLOB DOCUMENTATION_FILES "${CMAKE_SOURCE_DIR}/doc/Programmer/*.html.in" )

foreach( DOCFILE ${DOCUMENTATION_FILES} ) 
    get_filename_component( OUTPUT ${DOCFILE} NAME_WE )
    configure_file( ${DOCFILE} ${PROJECT_BINARY_DIR}/doc/Programmer/${OUTPUT}.html @ONLY )
endforeach()


# OD_BUILD_DOCUMENTATION - Make target "doc" to make documentation
macro( OD_BUILD_DOCUMENTATION )
    set( OD_DOXYGEN_PATH ${PROJECT_BINARY_DIR}/doc/Programmer/Generated )
    set( OD_DOXYGEN_FILE ${PROJECT_BINARY_DIR}/CMakeModules/Doxyfile )

    file ( GLOB DOXYGEN_PROGDOC_FILES "${CMAKE_SOURCE_DIR}/doc/Programmer/*.dox" )
    foreach ( OD_DOXYGEN_PROGDOC_FILE ${DOXYGEN_PROGDOC_FILES} )
	set ( OD_DOXYGEN_INPUT "${OD_DOXYGEN_INPUT} ${OD_DOXYGEN_PROGDOC_FILE}" )
    endforeach()

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
		${PROJECT_BINARY_DIR}/CMakeFiles/doxygenfooter.html @ONLY
		IMMEDIATE)

    OD_ADD_SOURCE_FILES( ${TEMPLATE} ${FOOTER} )

    set( OD_PROGDOC_URLPREFIX "http://doc.opendtect.org/${OpendTect_VERSION_MAJOR}${OpendTect_VERSION_MINOR}${OpendTect_VERSION_PATCH}/doc/Programmer"
	CACHE STRING "Online documentation prefix" )

    if ( UNIX AND OD_PROGDOC_URLPREFIX )
	set ( MAKE_SITEMAP_COMMAND
	    COMMAND ${OpendTect_DIR}/dtect/generate_progdoc_sitemap.sh
		    ${CMAKE_BINARY_DIR}/doc/Programmer/ ${OD_PROGDOC_URLPREFIX} )
    endif()

    add_custom_target ( doc
			${DOXYGEN_EXECUTABLE} ${OD_DOXYGEN_FILE}
			${MAKE_SITEMAP_COMMAND}
			SOURCES ${OD_DOXYGEN_FILE} )

    install ( DIRECTORY ${CMAKE_BINARY_DIR}/doc/Programmer/Generated/html DESTINATION
	                ${MISC_INSTALL_PREFIX}/doc/Programmer/Generated )
endmacro()

IF ( BUILD_DOCUMENTATION )
  find_package( Doxygen )
  if ( NOT DOXYGEN_FOUND )
    message( FATAL_ERROR 
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()

  if ( NOT DOXYGEN_DOT_FOUND )
    message( WARNING 
      "Dot is not found, but is needed to make the documentation graphs. Please install it correctly")
  endif()
endif()

if ( WIN32 )
    option( BUILD_USERDOC "Build user documentation" OFF )
endif()

if ( BUILD_USERDOC )
    if ( WIN32 )
        set( USERDOC_PROJECT "" CACHE FILEPATH "Path to user documentation project" )
	set( USERDOC_TARGET "HTML5" CACHE STRING "Documentation target" )
	find_program( MADCAP_FLARE_EXEC madbuild.exe
		  HINTS "C:/Program Files/MadCap Software/MadCap Flare 16/Flare.app"
			"E:/Program Files/MadCap Software/MadCap Flare 16/Flare.app"
		  DOC "Madcap Flare Executable"
             	  NO_DEFAULT_PATH )

	if ( NOT EXISTS ${MADCAP_FLARE_EXEC} )
	    message ( FATAL_ERROR "Madcap flare executable not found" )
	endif()

	if ( NOT EXISTS ${USERDOC_PROJECT} )
	    message ( FATAL_ERROR "Cannot find ${USERDOC_PROJECT}" )
	endif()

	set ( USER $ENV{USERNAME} )
	get_filename_component ( USERDOC_PROJECT_DIR ${USERDOC_PROJECT} PATH )
	get_filename_component ( USERDOC_NAME ${USERDOC_PROJECT} NAME_WE )

	set ( USERDOC_OUTPUT_DIR "${USERDOC_PROJECT_DIR}/Output/${USER}/${USERDOC_TARGET}" )

	add_custom_target( "userdoc" "${MADCAP_FLARE_EXEC}"
			    -project "${USERDOC_PROJECT}"
			    -target ${USERDOC_TARGET}
			    COMMENT "Building user documentation" )
	install( DIRECTORY ${USERDOC_OUTPUT_DIR}/ DESTINATION ${MISC_INSTALL_PREFIX}/doc/${USERDOC_NAME} )
    endif( WIN32 )

endif( BUILD_USERDOC )
