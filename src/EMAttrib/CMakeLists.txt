#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012/02/17 09:21:57 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS EarthModel AttributeEngine )
SET(OD_MODULE_SOURCES
		emhor2dseisiter.cc
		hor2dfrom3dcreator.cc
		seis2deventsnapper.cc
		gridcreator.cc
)
SET( OD_MODULE_BATCHPROGS od_process_2dgrid od_process_attrib_em )
OD_INIT_MODULE()


