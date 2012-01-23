#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS EarthModel AttributeEngine )
SET(OD_MODULE_SOURCES
		emhor2dseisiter.cc
		hor2dfrom3dcreator.cc
		seis2deventsnapper.cc
		gridcreator.cc
)
SET( OD_MODULE_BATCHPROGS od_process_2dgrid )
OD_INIT_MODULE()


