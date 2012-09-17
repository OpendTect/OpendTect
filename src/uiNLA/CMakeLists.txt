#_______________________Pmake___________________________________________________
#
#	Makefile : 	Non-Linear Analysis Ui
# 	Sep 2003	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo NLA Well Seis )
SET(OD_MODULE_SOURCES uinlapartserv.cc )
OD_INIT_MODULE()
