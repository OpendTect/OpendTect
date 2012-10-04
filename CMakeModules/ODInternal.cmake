#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________


#Configure odversion.h
configure_file ( ${OpendTect_DIR}/include/Basic/odversion.h.in ${OpendTect_DIR}/include/Basic/odversion.h )

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample
	DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION . PATTERN ".svn" EXCLUDE )

include ( ODSubversion )
