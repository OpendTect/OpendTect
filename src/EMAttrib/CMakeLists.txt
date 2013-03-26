#_______________________Pmake___________________________________________________
#
#	Makefile : 	Earth Model
# 	MNov 1999	Kris Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS EarthModel AttributeEngine )
set(OD_MODULE_SOURCES
		emhor2dseisiter.cc
		hor2dfrom3dcreator.cc
		seis2deventsnapper.cc
		gridcreator.cc
)
set( OD_MODULE_BATCHPROGS od_process_2dgrid.cc od_process_attrib_em.cc )
OD_INIT_MODULE()


