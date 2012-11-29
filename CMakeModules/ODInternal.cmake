#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.6 2012/09/11 06:14:31 cvsnageswara Exp $
#_______________________________________________________________________________

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN ".svn" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN ".svn" EXCLUDE )

#Install batchprogram example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/batchprogexample
		   DESTINATION doc/Programmer
		   PATTERN ".svn" EXCLUDE )

#install data folder
install( DIRECTORY ${CMAKE_SOURCE_DIR}/data 
	 DESTINATION .
	 PATTERN "install_files" EXCLUDE
	 PATTERN ".svn" EXCLUDE )

if ( UNIX )
    file ( GLOB TEXTFILES
	   ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
    file( GLOB PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list ( REMOVE_ITEM PROGRAMS
	   ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/.svn )
    foreach ( TEXTFILE ${TEXTFILES} )
	list ( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install ( PROGRAMS ${PROGRAMS} DESTINATION . )
    install ( FILES ${TEXTFILES} DESTINATION . )
endif( UNIX )

if ( APPLE )
   #Put in Info.plist
endif( APPLE )

if ( WIN32 )
#install runtime libraries
endif ( WIN32 )

add_custom_target( sources ${CMAKE_COMMAND}
	-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	-P ${CMAKE_SOURCE_DIR}/CMakeModules/ODInstallSources.cmake 
	 COMMENT "Installing sources" )

include ( ODSubversion )
