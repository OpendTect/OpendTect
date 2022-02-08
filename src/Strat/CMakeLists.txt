#_______________________Pmake___________________________________________________
#
#	Makefile : 	Stratigraphy
# 	Aug 2003	A.H. Bril
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General )
SET(OD_MODULE_SOURCES
	   initstrat.cc
	   stratlith.cc
	   stratlayer.cc
	   stratlayermodel.cc
	   stratlayersequence.cc
	   stratreftree.cc
	   stratseqattrib.cc
	   stratseqgen.cc
	   stratunit.cc
	   stratunititer.cc
	   stratunitrepos.cc
)

list ( APPEND OD_TEST_PROGS
        layermodel.cc
)

OD_INIT_MODULE()
