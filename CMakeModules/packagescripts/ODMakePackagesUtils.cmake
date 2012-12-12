#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       Nageswara
# Date:		August 2012		
#RCS:           $Id: ODMakePackagesUtils.cmake,v 1.11 2012/09/11 12:25:44 cvsnageswara Exp $

macro ( create_package PACKAGE_NAME )
    FILE( MAKE_DIRECTORY ${DESTINATION_DIR} ${DESTINATION_DIR}/bin
		         ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/plugins
			  ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )

    IF( ${PACKAGE_NAME} STREQUAL "base" )
        copy_thirdpartylibs()
        SET( LIBLIST ${LIBLIST};${PLUGINS} )
    ENDIF()

    MESSAGE( "Copying ${OD_PLFSUBDIR} libraries" )
    FOREACH ( FILE ${LIBLIST} )
	IF( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
		SET(LIB "lib${FILE}.so")
	ENDIF()

	IF( ${OD_PLFSUBDIR} STREQUAL "mac" )
		SET( LIB "lib${FILE}.dylib" )
	ENDIF()

	IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
		SET( LIB "${FILE}.dll" )
	ENDIF()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${LIB} 
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
       FILE( GLOB ALOFILES ${PSD}/plugins/${OD_PLFSUBDIR}/*.${FILE}.alo )
       FOREACH( ALOFILE ${ALOFILES} )
	   execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ALOFILE}
				    ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR} )
       ENDFOREACH()

    ENDFOREACH()

    IF( ${PACKAGE_NAME} STREQUAL "dgbbase" )
        SET( dgbdir "dgb${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${PSD}/../${dgbdir}/bin/${OD_PLFSUBDIR}/lm
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
    ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	SET( EXECLIST "${EXECLIST};${WINEXECLIST}" )
    ENDIF()

    MESSAGE( "Copying ${OD_PLFSUBDIR} executables" )
    FOREACH( EXE ${EXECLIST} )
	IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
		set( EXE "${EXE}.exe" )
	ENDIF()

	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${EXE} 
			 ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
    ENDFOREACH()

    IF( ${PACKAGE_NAME} STREQUAL "base" )
	# install SPECFILES files (from relbase/)
	FOREACH( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${CMAKE_INSTALL_PREFIX}/${SPECFILE}
			      ${DESTINATION_DIR} )
	ENDFOREACH()
	FOREACH( FILES ${ODSCRIPTS} )
	     FILE( GLOB SCRIPTS ${PSD}/bin/${FILES} )
	     FOREACH( SCRIPT ${SCRIPTS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					 ${DESTINATION_DIR}/bin )
	     ENDFOREACH()
	ENDFOREACH()
    ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${PSD}/bin/win/zip -r -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ELSE()
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ENDIF()

    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
    ENDIF()
    
    MESSAGE( "DONE" )

endmacro( create_package )


macro( copy_thirdpartylibs )
    MESSAGE( "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    FILE( GLOB LIBS ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/* )
    FOREACH( LIB ${LIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${LIB}
			  ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
    ENDFOREACH()

endmacro( copy_thirdpartylibs )


macro( create_basepackages PACKAGE_NAME )
   IF( ${PACKAGE_NAME} STREQUAL "basedata" )
       FOREACH( LIBS ${LIBLIST} )
	    FILE( GLOB DATAFILES ${CMAKE_INSTALL_PREFIX}/data/${LIBS} )
	    FOREACH( DATA ${DATAFILES} )
    #TODO if possible copy files instead of INSTALL
    #change file permissions if needed on windows
		  FILE( INSTALL DESTINATION ${DESTINATION_DIR}/data
				TYPE DIRECTORY FILES ${DATA}
				REGEX ".svn" EXCLUDE )
	    ENDFOREACH()
       ENDFOREACH()
   ENDIF()
   IF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
       FOREACH( LIB ${LIBLIST} )
	  IF( IS_DIRECTORY "${CMAKE_INSTALL_PREFIX}/data/${LIB}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  ELSE()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/data/${LIB}
			     ${DESTINATION_DIR}/data/${LIB} )
	  ENDIF()
       ENDFOREACH()
   ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${PSD}/bin/win/zip -r -q
					   "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ELSE()
	MESSAGE( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    ENDIF()

   IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
   ENDIF()
endmacro( create_basepackages )


macro( init_destinationdir  PACKAGE_NAME )
#    STRING ( TOUPPER ${PACKAGE_NAME} PACKAGE_NAME_UPPER )
#    SET ( FILELIST ${${PACKAGE_NAME_UPPER}_FILELIST} )

    SET ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    SET( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    IF( ${PACKAGE_NAME} STREQUAL "basedata" )
        SET( PACKAGE_FILENAME "basedata.zip" )
    ENDIF()
    IF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
        SET( PACKAGE_FILENAME "dgbbasedata.zip" )
    ENDIF()

    IF( NOT EXISTS ${PSD}/packages )
        FILE( MAKE_DIRECTORY ${PSD}/packages )
    ENDIF()

    SET( PACKAGE_DIR ${PSD}/packages )
    IF( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	FILE( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
    ENDIF()
    SET( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
    IF( APPLE )
	SET( REL_DIR "OpendTect${OpendTect_VERSION_MAJOR}
		      .${OpendTect_VERSION_MINOR}.app" )
        MESSAGE( "APPLE: reldiris ... ${REL_DIR}" )
    ENDIF()

    SET( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    IF( EXISTS ${DESTINATION_DIR} )
	FILE ( REMOVE_RECURSE ${DESTINATION_DIR} )
    ENDIF()

    FILE( MAKE_DIRECTORY ${DESTINATION_DIR} ${DESTINATION_DIR}/relinfo )
    SET( FULLVER_NAME "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
    SET( FULLVER_NAME "${FULLVER_NAME}${OpendTect_VERSION_PATCH}" )
    MESSAGE("full ver name:${FULLVER_NAME}")
    SET( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )

    IF( ${PACKAGE_NAME} STREQUAL "basedata" )
        SET( VER_FILENAME "basedata" )
    ENDIF()
    IF( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
        SET( VER_FILENAME "dgbbasedata" )
    ENDIF()

    FILE( WRITE ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
endmacro( init_destinationdir )


macro( create_develpackages )
    FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/doc )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${PSD}/CMakeLists.txt ${DESTINATION_DIR} )
    FOREACH( DIR CMakeModules include src plugins spec )
	Message( "Copying ${DIR} files" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${CMAKE_INSTALL_PREFIX}/${DIR}
			 ${DESTINATION_DIR}/${DIR} )
    ENDFOREACH()
    Message( "Copying Pmake stuff" )
    FOREACH( DIR ${PMAKESTUFF} )
	IF( IS_DIRECTORY "${PSD}/Pmake/${DIR}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${PSD}/Pmake/${DIR} ${DESTINATION_DIR}/Pmake/${DIR} )
	ELSE()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${PSD}/Pmake/${DIR} ${DESTINATION_DIR}/Pmake/${DIR} )
	ENDIF()
#//TODO File permissions are chaged after install. We need to copy as it is. 
#But '-E copy/copy_directory'option is working.
    ENDFOREACH()

    execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
			     WORKING_DIRECTORY ${PACKAGE_DIR}
			     RESULT_VARIABLE STATUS )
endmacro( create_develpackages )


macro( od_sign_maclibs )
    IF( APPLE )
	MESSAGE( "Signing mac libs..." )
	SET ( SIGN_ID "Developer ID Application: DGB-Earth Sciences B. V." )
	FILE( GLOB FILES ${CMAKE_INSTALL_PREFIX}/bin/mac/* )
	FOREACH( FIL ${FILES} )
	    execute_process( COMMAND  codesign -f -s ${SIGN_ID} ${FIL}
			     RESULT_VARIABLE STATUS )
	    IF( NOT STATUS EQUAL "0" )
		message("Failed")
	    ENDIF()
	ENDFOREACH()
    ENDIF()
    MESSAGE( "Done" )
endmacro( od_sign_maclibs )

macro( download_packages  )
message( "downloading doc pkgs" )
    SET( url  "http://intranet/documentations/rel/dgbdoc.zip" )
    FILE( DOWNLOAD ${url} "${CMAKE_INSTALL_PREFIX}/packages/dgbdoc.zip"
	  STATUS var
	  LOG log
	  SHOW_PROGRESS)
    MESSAGE( "status is:  ${var}" )
    IF( NOT var EQUAL "0" )
        MESSAGE( ".........Download Failed.........")
    ENDIF()
endmacro( download_packages )

#--------------------------------------------------------------
#Remove this macro
#Macro to get version name from the keyboard and stored in a file.
macro( getversion )
    execute_process( COMMAND ./parse_rel_number
		     WORKING_DIRECTORY /dsk/d21/nageswara/dev/od4.4/CMakeModules/packagescripts
		     RESULT_VARIABLE STATUS ) 
    IF( NOT ${STATUS} EQUAL "0" )
	message("Failed")
    ENDIF()
#//TODO How to remove '\n' while reading version name from the file?
#set(test "$ENV{WORK}")
    message("test version: ${fullv}")
    #set( VERSION_MAJOR "${OpendTect_FULL_VERSION}")
    #STRING( REGEX REPLACE "\n" "" VERS "${VERSION_MAJOR}" ) #//Not working
    #message("FULLVER:${OpendTect_FULL_VERSION} , Ver:${VERSION_MAJOR}")
endmacro( getversion )
