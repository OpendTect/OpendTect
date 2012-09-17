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

	MESSAGE( "modified file name:${LIB}" )
	MESSAGE( "modified :${PSD}" )
	FILE( INSTALL DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
		      TYPE FILE FILES ${PSD}/inst/bin/${OD_PLFSUBDIR}/${LIB} )

	FILE( GLOB ALOFILES ${PSD}/plugins/${OD_PLFSUBDIR}/*.${FILE}.alo )
	FOREACH( ALOFILE ${ALOFILES} )
	    FILE( INSTALL DESTINATION ${DESTINATION_DIR}/plugins/${OD_PLFSUBDIR}
			  TYPE FILE FILES ${ALOFILE} )
	ENDFOREACH()

    ENDFOREACH()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	SET( EXECLIST "${EXECLIST};${WINEXECLIST}" )
    ENDIF()

    FOREACH( EXE ${EXECLIST} )
	IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
		set( EXE "${EXE}.exe" )
	ENDIF()

	FILE( INSTALL DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
		      TYPE FILE FILES ${PSD}/inst/bin/${OD_PLFSUBDIR}/${EXE} )
    ENDFOREACH()

    IF( ${PACKAGE_NAME} STREQUAL "base" )
	# install SPECFILES files (from relbase/)
    ENDIF()

    IF( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	FILE( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
    ENDIF()

    execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
		       	     WORKING_DIRECTORY ${PACKAGE_DIR}
			     RESULT_VARIABLE STATUS )
    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
    ENDIF()
    
    MESSAGE( "DONE" )

endmacro( create_package )


macro( copy_thirdpartylibs )
    IF( NOT DEFINED QTDIR )
       MESSAGE( "FATAL_ERROR QTDIR is not defined" )
    ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
        SET( QTLIBS ${LUXQTLIBS} )
        SET( COINLIBS ${LUXCOINLIBS} )
        SET( OSGLIBS ${LUXOSGLIBS} )
    ELSEIF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
        SET( QTLIBS ${WINQTLIBS} )
        SET( COINLIBS ${WINCOINLIBS} )
        SET( OSGLIBS ${WINOSGLIBS} )
    ELSE()
        SET( QTLIBS ${MACQTLIBS} )
        SET( COINLIBS ${MACCOINLIBS} )
        SET( OSGLIBS ${MACOSGLIBS} )
    ENDIF()

    MESSAGE( "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    FOREACH( QTLIB ${QTLIBS} )
	FILE( GLOB QLIBS ${QTDIR}/lib/${QTLIB}*.[0-9] )
        FOREACH( QLIB ${QLIBS} )
            FILE( INSTALL DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
	                  TYPE FILE FILES ${QLIB} )
        ENDFOREACH()
    ENDFOREACH()

    FOREACH( COINLIB ${COINLIBS} )
        MESSAGE( "coin : ${COINDIR}" )
        MESSAGE( "coin libs: ${COINLIB}" )
	FILE( GLOB CLIBS ${COINDIR}/lib/${COINLIB}* )
        MESSAGE( "coin libs: ${CLIB}" )
        FOREACH( CLIB ${CLIBS} )
            FILE( INSTALL DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
	                  TYPE FILE FILES ${CLIB} )
        ENDFOREACH()
    ENDFOREACH()

    FOREACH( OSGLIB ${OSGLIBS} )
        FILE( INSTALL DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
	              TYPE FILE FILES ${OSG_DIR}/lib/${OSGLIB} )
    ENDFOREACH()
    
endmacro( copy_thirdpartylibs )


macro( create_basepackages )
   MESSAGE( "destdir:${DESTINATION_DIR}" )
   FILE( MAKE_DIRECTORY ${DESTINATION_DIR}/data
		        ${DESTINATION_DIR}/doc )
   FOREACH( LIBS ${LIBLIST} )
	FILE( GLOB DATAFILES ${PSD}/data/${LIBS} )
        MESSAGE( "datafiles: ${DATAFILES}" )
        FOREACH( DATA ${DATAFILES} )
	      MESSAGE( "datafile: ${DATA}" )
	      FILE( INSTALL DESTINATION ${DESTINATION_DIR}/data
			    TYPE DIRECTORY FILES ${DATA}
			    REGEX "CVS" EXCLUDE )
        ENDFOREACH()
   ENDFOREACH()

   execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
		       	    WORKING_DIRECTORY ${PACKAGE_DIR}
			    RESULT_VARIABLE STATUS )

   IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Could not zip file ${PACKAGE_FILENAME}" )
   ENDIF()
endmacro( create_basepackages )


macro( init_destinationdir  PACKAGE_NAME )
    STRING ( TOUPPER ${PACKAGE_NAME} PACKAGE_NAME_UPPER )
#    SET ( FILELIST ${${PACKAGE_NAME_UPPER}_FILELIST} )

    SET ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    IF( APPLE OR ${PACKAGE_NAME_UPPER}_PLFDEP )
	SET ( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}" )
    ENDIF()
    SET( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    IF( ${PACKAGE_NAME} STREQUAL "basedata" )
        SET( PACKAGE_FILENAME "basedata.zip" )
    ENDIF()

    IF( NOT EXISTS ${PSD}/packages )
        FILE( MAKE_DIRECTORY ${PSD}/packages )
    ENDIF()

    SET( PACKAGE_DIR ${PSD}/packages )
    SET( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
    SET( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    MESSAGE("Destdir:${DESTINATION_DIR}")
    MESSAGE("Reldir:${REL_DIR}")
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

    FILE( WRITE ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
endmacro( init_destinationdir )

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
