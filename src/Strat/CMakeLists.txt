#_______________________Pmake___________________________________________________
#
#	Makefile : 	Stratigraphy
# 	Aug 2003	Bert
#_______________________________________________________________________________


set(OD_MODULE_DEPS General )
set(OD_MODULE_SOURCES
	   stratlith.cc
	   stratunit.cc stratunititer.cc stratreftree.cc stratunitrepos.cc
	   stratlayer.cc stratseqgen.cc stratseqattrib.cc
	   initstrat.cc
)
OD_INIT_MODULE()
