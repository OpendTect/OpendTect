#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.6 2012/09/11 06:14:31 cvsnageswara Exp $
#_______________________________________________________________________________

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )

#Install batchprogram example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/batchprogexample
		   DESTINATION doc/Programmer
		   PATTERN "CVS" EXCLUDE )
