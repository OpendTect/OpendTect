#_______________________Pmake___________________________________________________
#
#	Makefile : 	EMAttrib
# 	Nov 1999	Kris
#_______________________________________________________________________________

set(OD_MODULE_DEPS MPEEngine)

set(OD_MODULE_SOURCES
	emhor2dseisiter.cc
	gridcreator.cc
	hor2dfrom3dcreator.cc
	horizonattrib.cc
	initemattrib.cc
	isopachmaker.cc
	seis2deventsnapper.cc
	stratamp.cc
)

set( OD_MODULE_BATCHPROGS
	od_isopach.cc
	od_process_2dgrid.cc
	od_process_attrib_em.cc
	od_stratamp.cc
)

OD_INIT_MODULE()
