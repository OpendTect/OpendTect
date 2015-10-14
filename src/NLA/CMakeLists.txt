#_______________________Pmake___________________________________________________
#
#	Makefile : 	Non-Linear Analysis interface
# 	Aug 2003	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________


add_definitions( -DmNoAutoUiString )

set(OD_MODULE_DEPS General )
set(OD_MODULE_SOURCES
    nlacrdesc.cc nladataprep.cc
)
OD_INIT_MODULE()



