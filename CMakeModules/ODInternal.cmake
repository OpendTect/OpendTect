#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.3 2012-05-15 15:22:52 cvshelene Exp $
#_______________________________________________________________________________

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION . PATTERN "CVS" EXCLUDE )
