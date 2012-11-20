#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Sep 2012	Nageawara
#	RCS :		$Id$
#_______________________________________________________________________________

#FILE( INSTALL DESTINATION ${PSD}/inst TYPE DIRECTORY FILES ${PSD}/plugins/${OD_PLFSUBDIR}
#	      REGEX "CVS" EXCLUDE )
#FILE( INSTALL DESTINATION ${PSD}/inst TYPE DIRECTORY FILES ${PSD}/data/icons.Classic
#	      REGEX "CVS" EXCLUDE )
#FILE( INSTALL DESTINATION ${PSD}/inst TYPE DIRECTORY FILES ${PSD}/data/icons.Default
#	      REGEX "CVS" EXCLUDE )
MESSAGE( "Installing ${OD_PLFSUBDIR} data and alofiles..." )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${PSD}/plugins/${OD_PLFSUBDIR}
		 				${PSD}/inst/${OD_PLFSUBDIR} )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${PSD}/data/icons.Classic
		 				${PSD}/inst/icons.Classic )
execute_process( COMMAND ${CMAKE_COMMAND} -E remove_directory ${PSD}/inst/icons.Classic/.svn )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${PSD}/data/icons.Default
		 				${PSD}/inst/icons.Default )
execute_process( COMMAND ${CMAKE_COMMAND} -E remove_directory ${PSD}/inst/icons.Default/.svn )

execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory ${PSD}/relbase
		 				${PSD}/inst/relbase )
execute_process( COMMAND ${CMAKE_COMMAND} -E remove_directory ${PSD}/inst/relbase/.svn )

SET( dgbdir "dgb${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${PSD}/../${dgbdir}/data/SequenceModels
		 				${PSD}/inst/data )

#installing thirdparty libs
IF( NOT EXISTS ${PSD}/inst/externallibs )
    FILE( MAKE_DIRECTORY ${PSD}/inst/externallibs )
ENDIF()

SET( EXTDIR ${PSD}/inst/externallibs )
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

MESSAGE( "Installing ${OD_PLFSUBDIR} thirdparty libraries..." )
MESSAGE( "Installing Qt libs..." )
FOREACH( QTLIB ${QTLIBS} )
    FOREACH( QLIB ${QTLIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${QTDIR}/lib/${QLIB} ${EXTDIR} )
    ENDFOREACH()
ENDFOREACH()
MESSAGE( "Installing Qt libs done." )

MESSAGE( "Installing coin libs..." )
FOREACH( COINLIB ${COINLIBS} )
    FILE( GLOB CLIBS ${COINDIR}/lib/${COINLIB}[0-9] )
    FOREACH( CLIB ${CLIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${CLIB} ${EXTDIR} )
    ENDFOREACH()
ENDFOREACH()
MESSAGE( "Installing coin libs done." )

MESSAGE( "Installing osg  libs..." )
FOREACH( OSGLIB ${OSGLIBS} )
    FILE( INSTALL DESTINATION ${EXTDIR}
		  TYPE FILE FILES ${OSG_DIR}/lib/${OSGLIB} )
ENDFOREACH()
MESSAGE( "Installing osg libs done." )
MESSAGE( "Release stuff installed successfully." )

