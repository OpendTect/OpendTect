#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.2 2012-03-29 11:18:33 cvskris Exp $
#_______________________________________________________________________________

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )
