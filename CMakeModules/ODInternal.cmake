#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.4 2012-05-18 09:06:36 cvskris Exp $
#_______________________________________________________________________________


#Configure odver.h
configure_file ( ${OpendTect_DIR}/include/Basic/odver.h.in ${OpendTect_DIR}/include/Basic/odver.h )

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN "CVS" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample DESTINATION doc/Programmer
	 PATTERN "CVS" EXCLUDE )

#Install data
install ( DIRECTORY "data" DESTINATION . PATTERN "CVS" EXCLUDE )
