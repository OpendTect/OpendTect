#_______________________Pmake___________________________________________________
#
#	Makefile : 	Non-Linear Analysis interface
# 	Aug 2003	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General )
SET(OD_MODULE_SOURCES
    nlacrdesc.cc nladataprep.cc
)
OD_INIT_MODULE()


